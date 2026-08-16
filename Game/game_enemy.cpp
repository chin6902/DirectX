/*============================================================================
Contents   :  [game_enemy.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/10
-----------------------------------------------------------------------------
One pool, one file. Enemy TYPES are rows in g_EnemyTypeInfo; enemy
BEHAVIOURS are cases in a switch. A new variant is a row - not a module.
============================================================================*/
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "game_enemy.h"
#include "game_player.h"
#include "game_stage.h"
#include "game_impact.h"
#include "texture.h"
#include "sprite.h"
#include "camera.h"
#include "collision_debug.h"

using namespace DirectX;

enum EnemyBehaviour
{
	BEHAVIOUR_CHASE,   
	BEHAVIOUR_ORBIT,   
};

struct EnemyTypeInfo
{
	EnemyBehaviour behaviour;
	float    speed;
	int      max_hp;
	float    radius;          
	float    draw_size;       
	int      contact_damage;
	XMFLOAT3 tint;
	float    orbit_radius;    
	float    knockback_scale; 
};

static constexpr EnemyTypeInfo g_EnemyTypeInfo[ENEMY_TYPE_COUNT] =
{
	// behaviour        speed    hp   radius draw   dmg   tint                   orbit  kb
	{ BEHAVIOUR_CHASE,  120.0f,  50,  26.0f, 64.0f,  1,  { 1.0f, 1.0f,  1.0f },   0.0f, 1.0f },
	{ BEHAVIOUR_ORBIT,  170.0f,  20,  22.0f, 56.0f,  1,  { 0.7f, 0.85f, 1.0f }, 220.0f, 1.3f },
};

static constexpr float KNOCKBACK_FRICTION = 0.88f; 
static constexpr float HIT_FLASH_TIME = 0.10f;
static constexpr float SEPARATION_STRENGTH = 90.0f;   
static constexpr float WANDER_RATE = 1.7f;   
static constexpr float WANDER_AMOUNT = 0.25f;  
static constexpr float BURN_TICK_INTERVAL = 0.5f;    

struct StatusEffect
{
	float time_left;
	float magnitude;    
	float tick_timer;   
};

struct Enemy
{
	int       id;               
	Vector2   pos;            
	EnemyType type;
	int       hp;
	float     orbit_dir;        

	// --- per-enemy variety ---
	float     speed_mul;        
	float     wander_phase;    
	float     orbit_mul;       

	// --- hit reaction state ---
	float     hitstun_timer;    
	float     knockback_timer;  
	Vector2   knockback_vel;
	float     flash_timer;      

	StatusEffect status[STATUS_TYPE_COUNT];

	bool      facing_left;
	bool      is_destroy;
};

static constexpr int MAX_ENEMIES = 300;
static Enemy g_Enemies[MAX_ENEMIES]{};
static int   g_EnemyCount = 0;
static int   g_NextEnemyId = 1;   

static int g_enemy_texture_id = -1;

void GameEnemy_Initialize()
{
	g_enemy_texture_id = Texture_Load(L"assets/textures/enemy.png");
	g_EnemyCount = 0;
	g_NextEnemyId = 1;   
}

void GameEnemy_Finalize()
{
	Texture_Release(g_enemy_texture_id);
	g_EnemyCount = 0;
}

void GameEnemy_Create(EnemyType type, const Vector2& pos)
{
	if (g_EnemyCount >= MAX_ENEMIES)
	{
		return;   
	}

	Enemy& e = g_Enemies[g_EnemyCount];
	e.id = g_NextEnemyId++;
	e.pos = pos;
	e.type = type;
	e.hp = g_EnemyTypeInfo[type].max_hp;
	e.orbit_dir = (g_EnemyCount & 1) ? 1.0f : -1.0f;      // alternate spin
	e.speed_mul = 0.85f + (rand() % 31) * 0.01f;          // 0.85 .. 1.15
	e.wander_phase = (rand() % 628) * 0.01f;              // 0 .. ~2PI
	e.orbit_mul = 0.75f + (rand() % 51) * 0.01f;          // 0.75 .. 1.25
	e.hitstun_timer = 0.0f;
	e.knockback_timer = 0.0f;
	e.knockback_vel = { 0.0f, 0.0f };
	e.flash_timer = 0.0f;
	for (StatusEffect& fx : e.status) { fx = { 0.0f, 0.0f, 0.0f }; }
	e.facing_left = false;
	e.is_destroy = false;
	g_EnemyCount++;
}

// ============================================================================
// Movement behaviours 
// ============================================================================
static Vector2 BehaviourChase(Enemy& e, const Vector2& to_player, float delta_time)
{
	e.wander_phase += delta_time * WANDER_RATE;

	const Vector2 straight = Vector2_Normalize(to_player);
	const Vector2 side = { -straight.y, straight.x };   // rotate 90 degrees

	// Weave: blend a little sideways drift into the straight-line approach.
	return Vector2_Normalize(straight + side * (sinf(e.wander_phase) * WANDER_AMOUNT));
}

static Vector2 BehaviourOrbit(const Enemy& e, const Vector2& to_player)
{
	const EnemyTypeInfo& info = g_EnemyTypeInfo[e.type];
	const float target_radius = info.orbit_radius * e.orbit_mul;

	const float dist = to_player.Length();
	const Vector2 inward = Vector2_Normalize(to_player);

	// Rotating (x,y) by 90 degrees gives (-y,x): that is the tangent.
	const Vector2 tangent = Vector2{ -inward.y, inward.x } * e.orbit_dir;

	// Blend: too far -> inward, too close -> outward, at the ring -> tangent.
	const float error = dist - target_radius;
	const float pull = std::clamp(error / target_radius, -1.0f, 1.0f);

	return Vector2_Normalize(inward * pull + tangent * (1.0f - fabsf(pull)));
}

static float UpdateStatus(Enemy& e, float delta_time)
{
	float speed_mul = e.speed_mul;

	for (int t = 0; t < STATUS_TYPE_COUNT; t++)
	{
		StatusEffect& fx = e.status[t];
		if (fx.time_left <= 0.0f)
		{
			continue;
		}

		fx.time_left -= delta_time;

		if (t == STATUS_BURN)
		{
			fx.tick_timer -= delta_time;
			if (fx.tick_timer <= 0.0f)
			{
				fx.tick_timer = BURN_TICK_INTERVAL;
				e.hp -= static_cast<int>(fx.magnitude);
				e.flash_timer = HIT_FLASH_TIME;

				if (e.hp <= 0 && !e.is_destroy)
				{
					e.is_destroy = true;
					GameImpact_Trigger(ExplosionType_Small, e.pos.x, e.pos.y);
					// LATER: XP gem spawn
				}
			}
		}
		else if (t == STATUS_SLOW)
		{
			speed_mul *= fx.magnitude;
		}
		else if (t == STATUS_FREEZE)
		{
			speed_mul = 0.0f;
		}
	}

	return speed_mul;
}

void GameEnemy_Update(float delta_time)
{
	const Vector2 player_pos = { GamePlayer_GetPosX(), GamePlayer_GetPosY() };

	// ---------------------------------------------------------------- 1..4
	for (int i = 0; i < g_EnemyCount; i++)
	{
		Enemy& e = g_Enemies[i];
		const EnemyTypeInfo& info = g_EnemyTypeInfo[e.type];

		if (e.flash_timer > 0.0f) { e.flash_timer -= delta_time; }

		const float speed_mul = UpdateStatus(e, delta_time);

		if (e.is_destroy)
		{
			continue; 
		}

		if (e.knockback_timer > 0.0f)
		{
			e.knockback_timer -= delta_time;

			const Vector2 old_pos = e.pos;
			e.pos += e.knockback_vel * delta_time;
			e.pos = GameStage_ResolvePosition(old_pos, e.pos, info.radius);

			e.knockback_vel *= KNOCKBACK_FRICTION;
		}

		if (e.hitstun_timer > 0.0f)
		{
			e.hitstun_timer -= delta_time;
			continue;
		}

		const Vector2 to_player = player_pos - e.pos;
		if (to_player.LengthSq() <= 1.0f)
		{
			continue;   
		}

		Vector2 dir{ 0.0f, 0.0f };
		switch (info.behaviour)
		{
		case BEHAVIOUR_CHASE: dir = BehaviourChase(e, to_player, delta_time); break;
		case BEHAVIOUR_ORBIT: dir = BehaviourOrbit(e, to_player);             break;
		}

		if (dir.IsZero())
		{
			continue;
		}

		const Vector2 old_pos = e.pos;
		e.pos += dir * (info.speed * speed_mul * delta_time);
		e.pos = GameStage_ResolvePosition(old_pos, e.pos, info.radius);

		e.facing_left = (dir.x < 0.0f);
	}

	// ------------------------------------------------------------------ 
	// Separation
	// This is the O(n^2) pass - the first thing that will want a spatial grid.
	for (int i = 0; i < g_EnemyCount; i++)
	{
		Enemy& a = g_Enemies[i];
		const float ra = g_EnemyTypeInfo[a.type].radius;

		for (int j = i + 1; j < g_EnemyCount; j++)
		{
			Enemy& b = g_Enemies[j];
			const float rb = g_EnemyTypeInfo[b.type].radius;

			const Vector2 d = b.pos - a.pos;
			const float min_dist = ra + rb;
			const float d_sq = d.LengthSq();

			// not overlapping, or exactly stacked (no direction to push along)
			if (d_sq >= min_dist * min_dist || d_sq < 0.01f)
			{
				continue;
			}

			const Vector2 push = Vector2_Normalize(d) * (SEPARATION_STRENGTH * delta_time);
			a.pos -= push;
			b.pos += push;
		}
	}

	// Separation into wall check
	for (int i = 0; i < g_EnemyCount; i++)
	{
		Enemy& e = g_Enemies[i];
		e.pos = GameStage_ResolvePosition(e.pos, e.pos, g_EnemyTypeInfo[e.type].radius);
	}
}

// ============================================================================
// Draw
// ============================================================================
void GameEnemy_Draw()
{
	for (int i = 0; i < g_EnemyCount; i++)
	{
		const Enemy& e = g_Enemies[i];
		const EnemyTypeInfo& info = g_EnemyTypeInfo[e.type];
		const float half = info.draw_size * 0.5f;

		SpriteDrawParams p;
		p.flip_x = e.facing_left;

		if (e.flash_timer > 0.0f)
		{
			p.color = { 2.5f, 2.5f, 2.5f };              
		}
		else if (e.status[STATUS_FREEZE].time_left > 0.0f)
		{
			p.color = { 0.45f, 0.75f, 1.60f };           
		}
		else if (e.status[STATUS_BURN].time_left > 0.0f)
		{
			p.color = { 1.60f, 0.55f, 0.20f };           
		}
		else if (e.status[STATUS_SLOW].time_left > 0.0f)
		{
			p.color = { 0.60f, 0.80f, 1.20f };       
		}
		else
		{
			p.color = info.tint;
		}

		Sprite_Draw(
			g_enemy_texture_id,
			Camera_WorldToScreenX(e.pos.x - half),
			Camera_WorldToScreenY(e.pos.y - half),
			info.draw_size, info.draw_size,
			p);
	}

#ifdef _DEBUG
	for (int i = 0; i < g_EnemyCount; i++)
	{
		const Enemy& e = g_Enemies[i];
		CollisionCircle cc = GameEnemy_GetCollisionCircle(i);
		cc.position.x = Camera_WorldToScreenX(cc.position.x);
		cc.position.y = Camera_WorldToScreenY(cc.position.y);

		const XMFLOAT3 col = (e.hitstun_timer > 0.0f) ? XMFLOAT3{ 1.0f, 1.0f, 0.0f } : XMFLOAT3{ 0.0f, 1.0f, 0.0f };
		Collision_Debug_Draw(cc, col);
	}
#endif
}

int GameEnemy_GetActiveCount()
{
	return g_EnemyCount;
}

int GameEnemy_GetId(int index)
{
	if (index < 0 || index >= g_EnemyCount)
	{
		return 0;   
	}
	return g_Enemies[index].id;
}

int GameEnemy_FindById(int id)
{
	if (id == 0) { return -1; }

	for (int i = 0; i < g_EnemyCount; i++)
	{
		if (g_Enemies[i].id == id) { return i; }
	}
	return -1;   
}

CollisionCircle GameEnemy_GetCollisionCircle(int index)
{
	if (index < 0 || index >= g_EnemyCount)
	{
		return { { 0.0f, 0.0f }, 0.0f };
	}

	const Enemy& e = g_Enemies[index];
	return { { e.pos.x, e.pos.y }, g_EnemyTypeInfo[e.type].radius };
}

Vector2 GameEnemy_GetPos(int index)
{
	if (index < 0 || index >= g_EnemyCount)
	{
		return { 0.0f, 0.0f };
	}
	return g_Enemies[index].pos;
}

int GameEnemy_GetContactDamage(int index)
{
	if (index < 0 || index >= g_EnemyCount)
	{
		return 0;
	}
	return g_EnemyTypeInfo[g_Enemies[index].type].contact_damage;
}

// ----------------------------------------------------------------------------
// ApplyHit: the enemy APPLIES what the attack DECIDED.
// Returns true only for the hit that actually kills, so the caller can
// award score / spawn drops exactly once.
// ----------------------------------------------------------------------------
bool GameEnemy_ApplyHit(int index, const HitInfo& hit)
{
	if (index < 0 || index >= g_EnemyCount)
	{
		return false;
	}

	Enemy& e = g_Enemies[index];
	if (e.is_destroy)
	{
		return false;   
	}

	const EnemyTypeInfo& info = g_EnemyTypeInfo[e.type];

	if (hit.knockback_speed > 0.0f && hit.knockback_time > 0.0f)
	{
		e.knockback_vel = hit.direction * (hit.knockback_speed * info.knockback_scale);

		e.knockback_timer = std::max(e.knockback_timer, hit.knockback_time);
	}
	e.hitstun_timer = std::max(e.hitstun_timer, hit.hitstun_time);
	e.flash_timer = HIT_FLASH_TIME;

	// --- Damage ---
	e.hp -= hit.damage;
	if (e.hp <= 0)
	{
		e.is_destroy = true;
		return true;
	}
	return false;
}

void GameEnemy_ApplyStatus(int index, StatusType type, float duration, float magnitude)
{
	if (index < 0 || index >= g_EnemyCount) { return; }
	if (type <= STATUS_NONE || type >= STATUS_TYPE_COUNT) { return; }

	StatusEffect& fx = g_Enemies[index].status[type];

	const bool was_active = (fx.time_left > 0.0f);

	if (!was_active)
	{
		fx.tick_timer = 0.0f;          
	}

	fx.time_left = std::max(fx.time_left, duration);

	if (!was_active)
	{
		fx.magnitude = magnitude;
	}
	else if (type == STATUS_SLOW)
	{
		fx.magnitude = std::min(fx.magnitude, magnitude);  
	}
	else
	{
		fx.magnitude = std::max(fx.magnitude, magnitude);  
	}
}

void GameEnemy_Destroy(int index)
{
	if (index < 0 || index >= g_EnemyCount)
	{
		return;
	}
	g_Enemies[index].is_destroy = true;
}

void GameEnemy_CleanUp()
{
	for (int i = g_EnemyCount - 1; i >= 0; --i)
	{
		if (g_Enemies[i].is_destroy)
		{
			g_Enemies[i] = g_Enemies[g_EnemyCount - 1];
			g_EnemyCount--;
		}
	}
}