/*============================================================================
Contents   :  [game_enemy.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/10
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "game_enemy.h"
#include "game_player.h"
#include "game_stage.h"
#include "game_impact.h"
#include "game_expgem.h"
#include "enemy_projectile.h"
#include "camera.h"
#include "config.h"
#include "texture.h"
#include "sprite.h"
#include "collision_debug.h"
#include "draw_primitives.h"
#include "enemy_formation.h"
#include "spatial_grid.h"
#include "game_damagenumber.h"
#include "flow_field.h"
#include "game_item.h"
#include "enemy_visual.h"
#include "game_audio.h"

using namespace DirectX;

enum EnemyBehaviour
{
	BEHAVIOUR_CHASE,   
	BEHAVIOUR_ORBIT,   
	BEHAVIOUR_CHARGE,
	BEHAVIOUR_FORMATION,
};

enum ChargeState
{
	CHARGE_APPROACH,    
	CHARGE_TELEGRAPH,  
	CHARGE_DASH,       
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
	int      xp_value;        

	// --- BEHAVIOUR_CHASE: (animation) ---
	float attack_range;
	float attack_cooldown;

	// --- BEHAVIOUR_ORBIT: shooting ---
	float    fire_interval;  
	float    shot_speed;
	int      shot_damage;

	// --- BEHAVIOUR_CHARGE: dashing ---
	float    charge_cooldown; 
	float    telegraph_time;  
	float    dash_speed;

	// --- elemental shield ---
	bool     has_shield;     

	float	 status_resist;
};

static constexpr EnemyTypeInfo g_EnemyTypeInfo[ENEMY_TYPE_COUNT] =
{
	// behaviour          //speed  //hp //radius //draw //cont_dmg //tint              //orbit //kb //xp  //atk_r/cd   //b   //b_spd //b_dmg //charge_cd //telegraph_time //dash_speed//has_shield //status_resist 
	// CHASER
	{ BEHAVIOUR_CHASE,    120.0f,  3,   26.0f,   76.0f,  2,       { 1.0f, 1.0f,  1.0f },   0.0f, 1.0f, 2, 52.0f, 1.2f, 0.0f,   0.0f,  0,     0.0f,       0.0f,            0.0f,       false,      1.0f },

	// ORBITER
	{ BEHAVIOUR_ORBIT,    170.0f,  2,   22.0f,   44.0f,  1,       { 0.7f, 0.85f, 1.0f }, 220.0f, 1.3f, 3, 0.0f, 0.0f, 2.60f, 420.0f,  1,     0.0f,       0.0f,            0.0f,       false,      1.0f },

	// ELITE
	{ BEHAVIOUR_CHARGE,   100.0f,  30,  34.0f,  100.0f,  2,       { 1.0f, 1.0f,  1.0f },   0.0f, 0.0f, 3, 0.0f, 0.0f,  0.0f,   0.0f,  0,    3.20f,      0.75f,         1150.0f,       true,       0.7f },

	// DUMMY
	{ BEHAVIOUR_FORMATION, 0.0f,   5,   24.0f,   68.0f,  3,       { 0.8f, 0.6f,  0.9f },   0.0f, 0.0f, 1, 0.0f, 0.0f,  0.0f,   0.0f,  0,     0.0f,       0.0f,            0.0f,       false,      0.0f },
};

static constexpr float KNOCKBACK_FRICTION = 0.88f; 
static constexpr float HIT_FLASH_TIME = 0.10f;
static constexpr float SEPARATION_STRENGTH = 90.0f;   
static constexpr float WANDER_RATE = 1.7f;   
static constexpr float WANDER_AMOUNT = 0.25f;  
static constexpr float BURN_TICK_INTERVAL = 0.5f;    
static constexpr float STUCK_THRESHOLD = 0.30f;
static constexpr float DETOUR_TIME = 0.90f;

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
	float     hp;
	float     orbit_dir;        

	// --- per-enemy variety ---
	float     speed_mul;        
	float     wander_phase;    
	float     orbit_mul;       

	// --- formation ---
	int		  formation_id;
	Vector2   formation_offset;

	// --- orbiter: shooting ---
	float     fire_timer;

	// --- hit reaction state ---
	float     hitstun_timer;    
	float     knockback_timer;  
	Vector2   knockback_vel;
	float     flash_timer;      

	// --- elite: charge state machine + shield ---
	ChargeState charge_state;
	float       charge_timer;
	Vector2     dash_dir;         
	float       dash_left;       
	bool        shield_up;
	ElementType shield_element;   

	StatusEffect status[STATUS_TYPE_COUNT];

	// stuck detection
	float     stuck_timer;
	float     detour_timer;
	Vector2   detour_dir;

	bool      facing_left;
	bool      is_destroy;

	// --- visual ---
	float	anim_timer;
	int		anim_frame;
	int		facing_row;
	int		last_clip;
	float   attack_timer;
	float   attack_cooldown;
	bool    attack_hit_done;
};

static constexpr int MAX_ENEMIES = 300;
static Enemy g_Enemies[MAX_ENEMIES]{};
static int   g_EnemyCount = 0;
static int   g_NextEnemyId = 1;   

static float g_HpScale = 1.0f;

void GameEnemy_Initialize()
{
	EnemyVisual_Initialize();
	g_EnemyCount = 0;
	g_NextEnemyId = 1;   
	g_HpScale = 1.0f;
}

void GameEnemy_Finalize()
{
	EnemyVisual_Finalize();	
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
	e.hp = std::max(1.0f, g_EnemyTypeInfo[type].max_hp * g_HpScale);
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
	e.anim_timer = 0.0f;
	e.anim_frame = rand() % 8;
	e.last_clip = ENEMY_CLIP_MOVE;
	e.attack_timer = 0.0f;
	e.attack_cooldown = 0.0f;
	const Vector2 to_player = GamePlayer_GetPos() - pos;
	e.facing_row = EnemyVisual_FacingFromDir(to_player);
	e.is_destroy = false;
	e.shield_up = g_EnemyTypeInfo[type].has_shield;
	e.shield_element = static_cast<ElementType>(rand() % ELEMENT_TYPE_COUNT);
	e.formation_id = 0;
	e.formation_offset = { 0.0f, 0.0f };
	e.fire_timer = 0.0f;
	e.charge_state = CHARGE_APPROACH;
	e.charge_timer = g_EnemyTypeInfo[type].charge_cooldown;
	e.dash_dir = { 0.0f, 0.0f };
	e.dash_left = 0.0f;
	e.stuck_timer = 0.0f;
	e.detour_timer = 0.0f;
	e.detour_dir = { 0.0f, 0.0f };
	g_EnemyCount++;
}

void GameEnemy_CreateInFormation(EnemyType type, int formation_id, const Vector2& offset)
{
	GameEnemy_Create(type, EnemyFormation_GetOrigin(formation_id) + offset);

	if (g_EnemyCount > 0)
	{
		Enemy& e = g_Enemies[g_EnemyCount - 1];
		e.formation_id = formation_id;
		e.formation_offset = offset;
	}
}

int GameEnemy_CountInFormation(int formation_id)
{
	int n = 0;
	for (int i = 0; i < g_EnemyCount; i++)
	{
		if (g_Enemies[i].formation_id == formation_id) { n++; }
	}
	return n;
}

// ============================================================================
// Movement behaviours 
// ============================================================================
static constexpr float FIELD_BLEND_DISTANCE = 140.0f;

static Vector2 RouteToPlayer(const Enemy& e, const Vector2& to_player)
{
	const Vector2 direct = Vector2_Normalize(to_player);

	const float dist = to_player.Length();
	if (dist < FIELD_BLEND_DISTANCE) { return direct; }

	const Vector2 flow = FlowField_GetDirection(e.pos);
	if (flow.IsZero()) { return direct; }      // unreachable

	const float t = std::clamp((dist - FIELD_BLEND_DISTANCE) / FIELD_BLEND_DISTANCE, 0.0f, 1.0f);

	return Vector2_Normalize(direct * (1.0f - t) + flow * t);
}

static Vector2 BehaviourChase(Enemy& e, const Vector2& to_player, float delta_time)
{
	e.wander_phase += delta_time * WANDER_RATE;

	const Vector2 straight = RouteToPlayer(e, to_player);
	const Vector2 side = { -straight.y, straight.x };  

	// blend a little sideways drift into the straight-line approach.
	return Vector2_Normalize(straight + side * (sinf(e.wander_phase) * WANDER_AMOUNT));
}

static void UpdateOrbiterFire(Enemy& e, const Vector2& to_player, float delta_time)
{
	const EnemyTypeInfo& info = g_EnemyTypeInfo[e.type];
	if (info.fire_interval <= 0.0f) { return; }

	e.fire_timer -= delta_time;
	if (e.fire_timer > 0.0f) { return; }

	const float dist = to_player.Length();
	const float target = info.orbit_radius * e.orbit_mul;

	if (fabsf(dist - target) > target * 0.45f)
	{
		e.fire_timer = 0.25f;
		return;
	}

	EnemyProjectile_Fire(e.pos, Vector2_Normalize(to_player),
		info.shot_speed, info.shot_damage,
		{ 0.75f, 0.85f, 1.0f });
	e.fire_timer = info.fire_interval;
}

static Vector2 BehaviourOrbit(const Enemy& e, const Vector2& to_player)
{
	const EnemyTypeInfo& info = g_EnemyTypeInfo[e.type];
	const float target_radius = info.orbit_radius * e.orbit_mul;

	const float dist = to_player.Length();
	const Vector2 inward = Vector2_Normalize(to_player);

	// Rotating (x,y) by 90 degrees
	const Vector2 tangent = Vector2{ -inward.y, inward.x } * e.orbit_dir;

	// Blend: too far -> inward, too close -> outward, at the ring -> tangent.
	const float error = dist - target_radius;
	const float pull = std::clamp(error / target_radius, -1.0f, 1.0f);

	return Vector2_Normalize(inward * pull + tangent * (1.0f - fabsf(pull)));
}

static Vector2 BehaviourCharge(Enemy& e, const Vector2& to_player, float delta_time)
{
	const EnemyTypeInfo& info = g_EnemyTypeInfo[e.type];

	switch (e.charge_state)
	{
	case CHARGE_APPROACH:
		e.charge_timer -= delta_time;
		if (e.charge_timer <= 0.0f)
		{
			e.charge_state = CHARGE_TELEGRAPH;
			e.charge_timer = info.telegraph_time;
		}
		return RouteToPlayer(e, to_player) * 0.45f;   // drift

	case CHARGE_TELEGRAPH:
		e.charge_timer -= delta_time;
		if (e.charge_timer <= 0.0f)
		{
			e.dash_dir = Vector2_Normalize(to_player);

			
			const float to_edge_x = (e.dash_dir.x > 0.0f) ? (Camera_GetX() + SCREEN_WIDTH - e.pos.x) : (e.pos.x - Camera_GetX());

			const float to_edge_y = (e.dash_dir.y > 0.0f) ? (Camera_GetY() + SCREEN_HEIGHT - e.pos.y) : (e.pos.y - Camera_GetY());

			const float tx = (fabsf(e.dash_dir.x) > 0.01f) ? to_edge_x / fabsf(e.dash_dir.x) : 99999.0f;
			const float ty = (fabsf(e.dash_dir.y) > 0.01f) ? to_edge_y / fabsf(e.dash_dir.y) : 99999.0f;

			e.dash_left = std::max(120.0f, std::min(tx, ty)); 
			e.charge_state = CHARGE_DASH;
		}
		return { 0.0f, 0.0f };

	case CHARGE_DASH:
	{
		const float step = info.dash_speed * delta_time;
		e.dash_left -= step;

		const bool off_screen =
			e.pos.x < Camera_GetX() - 120.0f ||
			e.pos.y < Camera_GetY() - 120.0f ||
			e.pos.x > Camera_GetX() + SCREEN_WIDTH + 120.0f ||
			e.pos.y > Camera_GetY() + SCREEN_HEIGHT + 120.0f;

		if (e.dash_left <= 0.0f || off_screen)
		{
			e.charge_state = CHARGE_APPROACH;
			e.charge_timer = info.charge_cooldown;
			return { 0.0f, 0.0f };
		}
		return e.dash_dir * (info.dash_speed / info.speed);   
	}
	}
	return { 0.0f, 0.0f };
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
				e.hp -= (fx.magnitude);
				e.flash_timer = HIT_FLASH_TIME;

				const float burn_damage = (fx.magnitude);
				GameDamageNumber_Spawn(e.pos, burn_damage, e.id, { 1.0f, 0.55f, 0.15f });

				if (e.hp <= 0 && !e.is_destroy)
				{
					e.is_destroy = true;
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

static void OnEnemyDeath(const Enemy& e)
{
	const EnemyTypeInfo& info = g_EnemyTypeInfo[e.type];

	if (e.type == ENEMY_TYPE_ELITE)
	{
		for (int i = 0; i < 12; i++)
		{
			const Vector2 p = e.pos + Vector2_FromAngle((rand() % 628) * 0.01f) * ((rand() % 90) + 20.0f);
			GameExpGem_Spawn(p, info.xp_value);
		}
		return;
	}

	GameItem_OnEnemyKilled(e.pos);
	GameExpGem_Spawn(e.pos, info.xp_value);
}

static float g_SeparationDelta = 0.0f;

static constexpr float SEPARATION_QUERY_RADIUS = 80.0f;

static void SeparatePair(int i, int j)
{
	Enemy& a = g_Enemies[i];
	Enemy& b = g_Enemies[j];

	if (a.formation_id != 0 || b.formation_id != 0) { return; }

	const float ra = g_EnemyTypeInfo[a.type].radius;
	const float rb = g_EnemyTypeInfo[b.type].radius;

	const Vector2 d = b.pos - a.pos;
	const float min_dist = ra + rb;
	const float d_sq = d.LengthSq();

	if (d_sq >= min_dist * min_dist || d_sq < 0.01f) { return; }

	const Vector2 push = Vector2_Normalize(d) * (SEPARATION_STRENGTH * g_SeparationDelta);
	a.pos -= push;
	b.pos += push;
}

static float AttackDuration(EnemyType type)
{
	return EnemyVisual_GetFrameCount(type, ENEMY_CLIP_ATTACK)
		* EnemyVisual_GetFrameTime(type, ENEMY_CLIP_ATTACK);
}

static EnemyClip ClipFor(const Enemy& e)
{
	if (e.attack_timer > 0.0f) { return ENEMY_CLIP_ATTACK; }
	if (g_EnemyTypeInfo[e.type].behaviour == BEHAVIOUR_CHARGE
		&& e.charge_state == CHARGE_DASH)
	{
		return ENEMY_CLIP_ATTACK;
	}
	return ENEMY_CLIP_MOVE;
}

static void UpdateAnim(Enemy& e, float delta_time)
{
	if (e.status[STATUS_FREEZE].time_left > 0.0f) { return; }

	const EnemyClip clip = ClipFor(e);
	const SoundId cycle = EnemyVisual_GetCycleSound(e.type, clip);

	if (clip != e.last_clip)
	{
		e.last_clip = clip;
		e.anim_frame = 0;
		e.anim_timer = 0.0f;
	}

	const float ft = EnemyVisual_GetFrameTime(e.type, clip);
	if (ft <= 0.0f) { return; }

	e.anim_timer += delta_time;

	const int frame_count = EnemyVisual_GetFrameCount(e.type, clip);

	while (e.anim_timer >= ft)
	{
		e.anim_timer -= ft;

		const int next = e.anim_frame + 1;
		e.anim_frame = next % frame_count;

		if (next >= frame_count)
		{
			GameAudio_PlayPitched(EnemyVisual_GetCycleSound(e.type, clip), 0.70f);
		}
	}
}

// ============================================================================
// Update
// ============================================================================
void GameEnemy_Update(float delta_time)
{
	const Vector2 player_pos = { GamePlayer_GetPosX(), GamePlayer_GetPosY() };

	for (int i = 0; i < g_EnemyCount; i++)
	{
		Enemy& e = g_Enemies[i];
		const EnemyTypeInfo& info = g_EnemyTypeInfo[e.type];

		if (e.flash_timer > 0.0f) { e.flash_timer -= delta_time; }

		const float speed_mul = UpdateStatus(e, delta_time);

		UpdateAnim(e, delta_time);

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

		if (info.behaviour == BEHAVIOUR_FORMATION)
		{
			if (EnemyFormation_IsActive(e.formation_id))
			{
				e.pos = EnemyFormation_GetOrigin(e.formation_id) + e.formation_offset;
			}
			else
			{
				e.is_destroy = true;  
			}
			continue;
		}

		const Vector2 to_player = player_pos - e.pos;
		if (to_player.LengthSq() <= 1.0f)
		{
			continue;   
		}

		if (info.attack_range > 0.0f)
		{
			if (e.attack_cooldown > 0.0f) { e.attack_cooldown -= delta_time; }

			if (e.attack_timer > 0.0f)
			{
				const float ft = EnemyVisual_GetFrameTime(e.type, ENEMY_CLIP_ATTACK);
				const float elapsed = AttackDuration(e.type) - e.attack_timer;
				const int   frame = (ft > 0.0f) ? static_cast<int>(elapsed / ft) : 0;

				// The arc is widest on frame 2 - that is the moment it lands.
				if (!e.attack_hit_done && frame >= 2)
				{
					e.attack_hit_done = true;
					if (to_player.LengthSq() < info.attack_range * info.attack_range)
					{
						PlayerHit hit;
						hit.damage = info.contact_damage;
						hit.knockback_speed = 400.0f;
						hit.knockback_time = 0.25f;
						hit.direction = Vector2_Normalize(to_player);
						GamePlayer_TakeHit(hit);
					}
				}

				e.attack_timer -= delta_time;
				if (e.attack_timer <= 0.0f)
				{
					e.attack_timer = 0.0f;
					e.attack_cooldown = info.attack_cooldown;
				}
				continue;   
			}

			if (e.attack_cooldown <= 0.0f
				&& to_player.LengthSq() < info.attack_range * info.attack_range)
			{
				e.attack_timer = AttackDuration(e.type);
				e.attack_hit_done = false;      // arm the next swing
				e.facing_row = EnemyVisual_FacingFromDir(to_player);
				GameAudio_Play(SND_ORC_SWING);
				continue;
			}
		}

		Vector2 dir{ 0.0f, 0.0f };
		switch (info.behaviour)
		{
		case BEHAVIOUR_CHASE:
			dir = BehaviourChase(e, to_player, delta_time);
			break;

		case BEHAVIOUR_ORBIT:
			dir = BehaviourOrbit(e, to_player);
			UpdateOrbiterFire(e, to_player, delta_time);
			break;

		case BEHAVIOUR_CHARGE:
			dir = BehaviourCharge(e, to_player, delta_time);
			break;
		}

		if (dir.IsZero())
		{
			continue;
		}

		// --- unstuck detour ---
			if (e.detour_timer > 0.0f)
			{
				e.detour_timer -= delta_time;
				dir = e.detour_dir;
			}

		const Vector2 old_pos = e.pos;
		e.pos += dir * (info.speed * speed_mul * delta_time);
		e.pos = GameStage_ResolvePosition(old_pos, e.pos, info.radius);

		const float wanted = info.speed * speed_mul * delta_time;
		const float moved = (e.pos - old_pos).Length();

		if (wanted > 0.1f && moved < wanted * 0.35f)
		{
			e.stuck_timer += delta_time;

			if (e.stuck_timer > STUCK_THRESHOLD && e.detour_timer <= 0.0f)
			{
				e.detour_dir = (e.orbit_dir > 0.0f) ? Vector2{ -dir.y,  dir.x } : Vector2{ dir.y, -dir.x };
				e.detour_timer = DETOUR_TIME;
				e.stuck_timer = 0.0f;
			}
		}
		else
		{
			e.stuck_timer = 0.0f;
		}

		e.facing_left = (dir.x < 0.0f);

		if (info.behaviour != BEHAVIOUR_FORMATION)
		{
			e.facing_row = EnemyVisual_FacingFromDir(dir);
		}
	}

	// --- Separation ---
	GameEnemy_BuildGrid();

	g_SeparationDelta = delta_time;
	SpatialGrid_ForEachPair(SEPARATION_QUERY_RADIUS, SeparatePair);

	// Separation into wall check
	for (int i = 0; i < g_EnemyCount; i++)
	{
		Enemy& e = g_Enemies[i];
		if (e.formation_id != 0) { continue; }
		e.pos = GameStage_ResolvePosition(e.pos, e.pos, g_EnemyTypeInfo[e.type].radius);
	}
}

// ============================================================================
// Draw
// ============================================================================
static XMFLOAT3 ElementColor(int e)
{
	switch (e)
	{
	case ELEMENT_FIRE:    return { 1.00f, 0.35f, 0.10f };
	case ELEMENT_ICE:     return { 0.30f, 0.70f, 1.00f };
	case ELEMENT_THUNDER: return { 1.00f, 0.90f, 0.20f };
	default:              return { 0.35f, 0.35f, 0.35f };
	}
}

void GameEnemy_Draw()
{
	for (int i = 0; i < g_EnemyCount; i++)
	{
		const Enemy& e = g_Enemies[i];
		const EnemyTypeInfo& info = g_EnemyTypeInfo[e.type];

		SpriteDrawParams p;

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

		const EnemyClip clip = ClipFor(e);

		EnemyVisual_Draw(e.type, clip, e.anim_frame, e.facing_row, e.pos, info.draw_size, p);

		if (e.shield_up)
		{
			const XMFLOAT3 col = ElementColor(e.shield_element);
			DrawPrim_Ring(e.pos, info.radius + 12.0f, 3.0f, col, 0.9f);
			DrawPrim_Ring(e.pos, info.radius + 6.0f, 1.5f, col, 0.45f);
		}

		if (info.behaviour == BEHAVIOUR_CHARGE && e.charge_state == CHARGE_TELEGRAPH)
		{
			const float t = 1.0f - (e.charge_timer / info.telegraph_time);
			const Vector2 aim = Vector2_Normalize(
				Vector2{ GamePlayer_GetPosX(), GamePlayer_GetPosY() } - e.pos);

			DrawPrim_Line(e.pos, e.pos + aim * (520.0f * t), 5.0f,
				{ 1.0f, 0.35f, 0.35f }, 0.30f + 0.45f * t);
		}
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

	if (e.shield_up)
	{
		if ((hit.element_mask & ElementBit(e.shield_element)) == 0)
		{
			e.flash_timer = HIT_FLASH_TIME;   
			return false;                     
		}

		e.shield_up = false;          
	}

	if (hit.knockback_speed > 0.0f && hit.knockback_time > 0.0f)
	{
		e.knockback_vel = hit.direction * (hit.knockback_speed * info.knockback_scale);

		e.knockback_timer = std::max(e.knockback_timer, hit.knockback_time);
	}

	if (info.behaviour == BEHAVIOUR_FORMATION) 
	{
		e.hitstun_timer = 0;
	}
	else
	{
		e.hitstun_timer = std::max(e.hitstun_timer, hit.hitstun_time);
	}

	e.flash_timer = HIT_FLASH_TIME;

	// --- Damage ---
	e.hp -= hit.damage;
	if (e.hp <= 0)
	{
		e.is_destroy = true;
		OnEnemyDeath(e);
		return true;
	}
	GameAudio_Play(SND_ENEMY_DIE);
	return false;
}

void GameEnemy_ApplyStatus(int index, StatusType type, float duration, float magnitude)
{
	if (index < 0 || index >= g_EnemyCount) { return; }
	if (type <= STATUS_NONE || type >= STATUS_TYPE_COUNT) { return; }

	Enemy& target = g_Enemies[index];

	if (type == STATUS_SLOW)
	{
		duration *= g_EnemyTypeInfo[target.type].status_resist;
	}

	if (type == STATUS_FREEZE)
	{
		duration *= g_EnemyTypeInfo[target.type].status_resist;
	}

	if (duration <= 0.0f) { return; }

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

bool GameEnemy_HasShield(int index)
{
	if (index < 0 || index >= g_EnemyCount) { return false; }
	return g_Enemies[index].shield_up;
}

ElementType GameEnemy_GetShieldElement(int index)
{
	if (index < 0 || index >= g_EnemyCount) { return ELEMENT_FIRE; }
	return g_Enemies[index].shield_element;
}

int GameEnemy_CountOfType(EnemyType type)
{
	int n = 0;
	for (int i = 0; i < g_EnemyCount; i++)
	{
		if (g_Enemies[i].type == type) { n++; }
	}
	return n;
}

bool GameEnemy_UsesAttack(int index)
{
	if (index < 0 || index >= g_EnemyCount) { return false; }
	return g_EnemyTypeInfo[g_Enemies[index].type].attack_range > 0.0f;
}

int GameEnemy_QueryRadius(const Vector2& center, float radius, int* out, int out_max)
{
	return SpatialGrid_Query(center, radius, out, out_max);
}

void GameEnemy_SetHPScale(float scale)
{
	g_HpScale = std::max(0.1f, scale);
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

void GameEnemy_BuildGrid()
{
	SpatialGrid_Clear();
	for (int i = 0; i < g_EnemyCount; i++)
	{
		SpatialGrid_Insert(i, g_Enemies[i].pos);
	}
}
