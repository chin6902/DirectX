/*============================================================================
Contents   :  [game_boss.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/19
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "game_boss.h"
#include "health.h"
#include "game_player.h"
#include "game_time.h"
#include "game_expgem.h"
#include "game_impact.h"
#include "enemy_formation.h"
#include "camera.h"
#include "config.h"
#include "texture.h"
#include "sprite.h"
#include "draw_primitives.h"
#include "game_text.h"
#include "game_ui.h"
#include "game_damagenumber.h"
#include "game_audio.h"

using namespace DirectX;

static constexpr int   BOSS_VARIANT_COUNT = 2;
static constexpr float BOSS_MAX_HP[BOSS_VARIANT_COUNT] = { 750.0f, 950.0f };

static constexpr float BOSS_RADIUS = 52.0f;
static constexpr float BOSS_DRAW_SIZE = 160.0f;

static constexpr float APPROACH_SPEED = 100.0f;
static constexpr float APPROACH_TIME = 2.20f;
static constexpr float TELEGRAPH_TIME = 1.00f;
static constexpr float AIRBORNE_TIME = 0.75f;
static constexpr float LAND_RECOVER_TIME = 0.90f;
static constexpr float DEATH_TIME = 1.20f;

static constexpr float SLAM_RADIUS = 150.0f;
static constexpr int   SLAM_DAMAGE = 3;
static constexpr float SLAM_KNOCKBACK = 520.0f;

static constexpr float JUMP_ARC_HEIGHT = 190.0f;   
static constexpr int   JUMPS_PER_SHIELD = 3;

static constexpr float SHAKE_STRENGTH = 26.0f;
static constexpr float SHAKE_TIME = 0.35f;

static constexpr float PHASE_2_HP = 0.75f;
static constexpr float PHASE_3_HP = 0.50f;
static constexpr float SUMMON_INTERVAL_P2 = 9.0f;
static constexpr float SUMMON_INTERVAL_P3 = 7.0f;

static constexpr float BOSS_STATUS_RESIST = 0.4f;
static constexpr float BURN_TICK_INTERVAL = 0.5f;
static constexpr float CLOCK_FLOOR = 0.25f;   

static constexpr float STAGGER_PER_INDEX = 1.10f;

static constexpr int   DEATH_GEM_COUNT = 30;
static constexpr int   DEATH_GEM_VALUE = 6;

// --- sprite sheets ---
static constexpr int SHEET_CELL = 64;
static constexpr int IDLE_FRAMES = 6;
static constexpr int RUN_FRAMES = 8;
static constexpr int ATK_FRAMES = 11;
static constexpr int DEATH_FRAMES = 10;

static constexpr int ROW_DOWN = 0;
static constexpr int ROW_UP = 1;
static constexpr int ROW_LEFT = 2;
static constexpr int ROW_RIGHT = 3;

enum BossState
{
	BOSS_APPROACH,
	BOSS_TELEGRAPH,
	BOSS_AIRBORNE,
	BOSS_LAND,
	BOSS_DEAD,
};

struct BossStatus
{
	float time_left = 0.0f;
	float magnitude = 0.0f;
	float tick_timer = 0.0f;
};

struct Boss
{
	bool        active = false;
	int         variant = 0;      
	bool        summons = true;   

	BossState   state = BOSS_APPROACH;
	float       state_timer = 0.0f;

	Health      health;
	Vector2     pos;
	Vector2     jump_from;
	Vector2     jump_to;
	float       jump_height = 0.0f;

	float       flash_timer = 0.0f;

	int         jump_count = 0;
	bool        shield_up = false;
	ElementType shield_element = ELEMENT_FIRE;

	int         phase = 1;
	float       summon_timer = 0.0f;
	bool        wall_from_left = true;

	float       anim_timer = 0.0f;
	int         anim_frame = 0;

	BossStatus  status[STATUS_TYPE_COUNT];
};

static Boss g_Bosses[BOSS_MAX]{};

static int g_tex_idle[BOSS_VARIANT_COUNT] = { -1, -1 };
static int g_tex_run[BOSS_VARIANT_COUNT] = { -1, -1 };
static int g_tex_atk[BOSS_VARIANT_COUNT] = { -1, -1 };
static int g_tex_death[BOSS_VARIANT_COUNT] = { -1, -1 };

static bool ValidIndex(int index)
{
	return index >= 0 && index < BOSS_MAX && g_Bosses[index].active;
}

static void RollShield(Boss& b)
{
	b.shield_element = static_cast<ElementType>(rand() % ELEMENT_TYPE_COUNT);
	b.shield_up = true;
	b.jump_count = 0;
}

static XMFLOAT3 ElementColor(ElementType e)
{
	switch (e)
	{
	case ELEMENT_FIRE:    return { 1.00f, 0.35f, 0.10f };
	case ELEMENT_ICE:     return { 0.30f, 0.70f, 1.00f };
	case ELEMENT_THUNDER: return { 1.00f, 0.90f, 0.20f };
	default:              return { 1.00f, 1.00f, 1.00f };
	}
}

static int FacingRow(const Vector2& dir)
{
	if (fabsf(dir.x) > fabsf(dir.y)) { return (dir.x > 0.0f) ? ROW_RIGHT : ROW_LEFT; }
	return (dir.y > 0.0f) ? ROW_DOWN : ROW_UP;
}

static void EnterDeath(Boss& b)
{
	b.state = BOSS_DEAD;
	b.state_timer = DEATH_TIME;
	b.anim_frame = 0;
	b.anim_timer = 0.0f;
}

void GameBoss_Initialize()
{
	g_tex_idle[0] = Texture_Load(L"assets/textures/Slime3_Idle_full.png");
	g_tex_run[0] = Texture_Load(L"assets/textures/Slime3_Run_full.png");
	g_tex_atk[0] = Texture_Load(L"assets/textures/Slime3_Attack_full.png");
	g_tex_death[0] = Texture_Load(L"assets/textures/Slime3_Death_full.png");

	g_tex_idle[1] = Texture_Load(L"assets/textures/Slime2_Idle_full.png");
	g_tex_run[1] = Texture_Load(L"assets/textures/Slime2_Run_full.png");
	g_tex_atk[1] = Texture_Load(L"assets/textures/Slime2_Attack_full.png");
	g_tex_death[1] = Texture_Load(L"assets/textures/Slime2_Death_full.png");

	for (Boss& b : g_Bosses) { b.active = false; }
}

void GameBoss_Finalize()
{
	for (int i = 0; i < BOSS_VARIANT_COUNT; i++)
	{
		Texture_Release(g_tex_idle[i]);
		Texture_Release(g_tex_run[i]);
		Texture_Release(g_tex_atk[i]);
		Texture_Release(g_tex_death[i]);
	}
	for (Boss& b : g_Bosses) { b.active = false; }
}

void GameBoss_Spawn(const Vector2& pos, int variant, bool summons)
{
	int slot = -1;
	for (int i = 0; i < BOSS_MAX; i++)
	{
		if (!g_Bosses[i].active) { slot = i; break; }
	}
	if (slot < 0) { return; }   

	Boss& b = g_Bosses[slot];

	b.variant = std::clamp(variant - 1, 0, BOSS_VARIANT_COUNT - 1);
	b.summons = summons;

	b.health.Reset(BOSS_MAX_HP[b.variant]);

	b.pos = pos;
	b.jump_from = pos;
	b.jump_to = pos;
	b.jump_height = 0.0f;
	b.flash_timer = 0.0f;

	b.state = BOSS_APPROACH;

	b.state_timer = APPROACH_TIME + STAGGER_PER_INDEX * slot;

	b.phase = 1;
	b.summon_timer = SUMMON_INTERVAL_P2;
	b.wall_from_left = true;
	b.anim_timer = 0.0f;
	b.anim_frame = 0;

	for (BossStatus& fx : b.status) { fx = { 0.0f, 0.0f, 0.0f }; }

	RollShield(b);
	b.active = true;
}

bool    GameBoss_IsActive(int index) { return ValidIndex(index); }
Vector2 GameBoss_GetPos(int index) { return ValidIndex(index) ? g_Bosses[index].pos : Vector2{ 0.0f, 0.0f }; }
float   GameBoss_GetRadius(int index) { return BOSS_RADIUS; }

float GameBoss_GetHPFraction(int index)
{
	return ValidIndex(index) ? g_Bosses[index].health.Fraction() : 0.0f;
}

bool GameBoss_HasShield(int index)
{
	return ValidIndex(index) && g_Bosses[index].shield_up;
}

ElementType GameBoss_GetShieldElement(int index)
{
	return ValidIndex(index) ? g_Bosses[index].shield_element : ELEMENT_FIRE;
}

int GameBoss_GetActiveCount()
{
	int n = 0;
	for (const Boss& b : g_Bosses) { if (b.active) { n++; } }
	return n;
}

bool GameBoss_AnyActive() { return GameBoss_GetActiveCount() > 0; }

// ============================================================================
// Damage
// ============================================================================
bool GameBoss_ApplyHit(int index, const HitInfo& hit)
{
	if (!ValidIndex(index)) { return false; }

	Boss& b = g_Bosses[index];
	if (b.health.IsDead()) { return false; }

	if (b.shield_up)
	{
		if ((hit.element_mask & ElementBit(b.shield_element)) == 0)
		{
			return false;
		}
		b.shield_up = false;
		GameTime_Hitstop(0.08f);      
	}

	b.flash_timer = 0.10f;

	if (b.health.Damage(hit.damage))
	{
		EnterDeath(b);
		return true;
	}
	return false;
}

void GameBoss_ApplyStatus(int index, StatusType type, float duration, float magnitude)
{
	if (!ValidIndex(index)) { return; }
	if (type <= STATUS_NONE || type >= STATUS_TYPE_COUNT) { return; }

	Boss& b = g_Bosses[index];
	if (b.health.IsDead()) { return; }

	if (type == STATUS_SLOW || type == STATUS_FREEZE)
	{
		duration *= BOSS_STATUS_RESIST;
	}

	BossStatus& fx = b.status[type];
	const bool was_active = (fx.time_left > 0.0f);

	if (!was_active) { fx.tick_timer = 0.0f; }
	fx.time_left = std::max(fx.time_left, duration);

	if (!was_active) { fx.magnitude = magnitude; }
	else if (type == STATUS_SLOW) { fx.magnitude = std::min(fx.magnitude, magnitude); }
	else { fx.magnitude = std::max(fx.magnitude, magnitude); }
}

// ============================================================================
// Updates
// ============================================================================
static float UpdateStatus(Boss& b, float delta_time)
{
	float speed_mul = 1.0f;
	const int index = static_cast<int>(&b - g_Bosses);

	for (int t = 0; t < STATUS_TYPE_COUNT; t++)
	{
		BossStatus& fx = b.status[t];
		if (fx.time_left <= 0.0f) { continue; }

		fx.time_left -= delta_time;

		if (t == STATUS_BURN)
		{
			fx.tick_timer -= delta_time;
			if (fx.tick_timer <= 0.0f)
			{
				fx.tick_timer = BURN_TICK_INTERVAL;
				b.flash_timer = 0.08f;

				const float burn_damage = fx.magnitude;

				GameDamageNumber_Spawn(b.pos, burn_damage, -100 - index, { 1.0f, 0.55f, 0.15f });

				if (b.health.Damage(fx.magnitude))
				{
					EnterDeath(b);   
					return speed_mul;
				}
			}
		}
		else if (t == STATUS_SLOW) { speed_mul *= fx.magnitude; }
		else if (t == STATUS_FREEZE) { speed_mul = 0.0f; }
	}

	return speed_mul;
}

static void UpdatePhase(Boss& b, float delta_time)
{
	const float frac = b.health.Fraction();

	if (b.phase == 1 && frac <= PHASE_2_HP)
	{
		b.phase = 2;
		b.summon_timer = 1.0f;                
		Camera_Shake(18.0f, 0.5f);
	}
	else if (b.phase == 2 && frac <= PHASE_3_HP)
	{
		b.phase = 3;
		b.summon_timer = 1.0f;
		Camera_Shake(24.0f, 0.7f);
	}

	if (!b.summons || b.phase < 2) { return; }

	b.summon_timer -= delta_time;
	if (b.summon_timer > 0.0f) { return; }

	const Vector2 player = GamePlayer_GetPos();

	if (b.phase == 2)
	{
		const Vector2 dir = Vector2_FromAngle((rand() % 628) * 0.01f);
		EnemyFormation_SpawnTriangle(player - dir * 900.0f, dir, 260.0f, 4);
		b.summon_timer = SUMMON_INTERVAL_P2;
	}
	else
	{
		if (b.wall_from_left)
		{
			EnemyFormation_SpawnWall({ Camera_GetX() - 120.0f, player.y }, +1.0f, 190.0f);
		}
		else
		{
			EnemyFormation_SpawnWall({ Camera_GetX() + SCREEN_WIDTH + 120.0f, player.y }, -1.0f, 190.0f);
		}
		b.wall_from_left = !b.wall_from_left;
		b.summon_timer = SUMMON_INTERVAL_P3;
	}
}

static void UpdateOne(Boss& b, float delta_time)
{
	if (b.flash_timer > 0.0f) { b.flash_timer -= delta_time; }

	b.anim_timer += delta_time;
	if (b.anim_timer >= 0.09f)
	{
		b.anim_timer -= 0.09f;
		b.anim_frame++;
	}

	if (b.state == BOSS_DEAD)
	{
		b.state_timer -= delta_time;
		if (b.state_timer <= 0.0f)
		{
			for (int i = 0; i < DEATH_GEM_COUNT; i++)
			{
				const Vector2 p = b.pos
					+ Vector2_FromAngle((rand() % 628) * 0.01f)
					* static_cast<float>((rand() % 140) + 30);
				GameExpGem_Spawn(p, DEATH_GEM_VALUE);
			}

			Camera_Shake(30.0f, 0.6f);
			b.active = false;
		}
		return;
	}

	const float speed_mul = UpdateStatus(b, delta_time);
	if (b.state == BOSS_DEAD) { return; }      

	UpdatePhase(b, delta_time);

	const float clock = std::max(speed_mul, CLOCK_FLOOR);

	const Vector2 player = GamePlayer_GetPos();
	b.state_timer -= delta_time * clock;

	switch (b.state)
	{
	case BOSS_APPROACH:
	{
		const Vector2 to_player = player - b.pos;
		if (to_player.LengthSq() > 1.0f)
		{
			b.pos += Vector2_Normalize(to_player) * (APPROACH_SPEED * speed_mul * delta_time);
		}

		if (b.state_timer <= 0.0f)
		{
			b.state = BOSS_TELEGRAPH;
			b.state_timer = TELEGRAPH_TIME;

			b.jump_to = player;
			b.jump_from = b.pos;
			b.anim_frame = 0;
		}
		break;
	}

	case BOSS_TELEGRAPH:
		if (b.state_timer <= 0.0f)
		{
			b.state = BOSS_AIRBORNE;
			b.state_timer = AIRBORNE_TIME;
			b.anim_frame = 0;
		}
		break;

	case BOSS_AIRBORNE:
	{
		const float t = 1.0f - std::max(0.0f, b.state_timer / AIRBORNE_TIME);

		b.pos = Vector2_Lerp(b.jump_from, b.jump_to, t);

		// a parabola for the visual lift
		b.jump_height = JUMP_ARC_HEIGHT * (4.0f * t * (1.0f - t));

		if (b.state_timer <= 0.0f)
		{
			b.pos = b.jump_to;
			b.jump_height = 0.0f;
			b.state = BOSS_LAND;
			b.state_timer = LAND_RECOVER_TIME;
			b.anim_frame = 0;

			// --- the slam ---
			Camera_Shake(SHAKE_STRENGTH, SHAKE_TIME);
			GameTime_Hitstop(0.05f);
			GameAudio_Play(SND_BOSS_LAND);

			const Vector2 to_player = player - b.pos;
			if (to_player.LengthSq() < SLAM_RADIUS * SLAM_RADIUS)
			{
				PlayerHit hit;
				hit.damage = SLAM_DAMAGE;
				hit.knockback_speed = SLAM_KNOCKBACK;
				hit.knockback_time = 0.25f;
				hit.direction = (to_player.LengthSq() > 0.01f)
					? Vector2_Normalize(to_player)
					: Vector2{ 1.0f, 0.0f };
				GamePlayer_TakeHit(hit);
			}

			// the shield returns every few jumps
			if (++b.jump_count >= JUMPS_PER_SHIELD)
			{
				RollShield(b);
			}
		}
		break;
	}

	case BOSS_LAND:
		if (b.state_timer <= 0.0f)
		{
			b.state = BOSS_APPROACH;

			b.state_timer = APPROACH_TIME * ((b.phase >= 3) ? 0.6f : (b.phase == 2) ? 0.8f : 1.0f);
			b.anim_frame = 0;
		}
		break;

	default:
		break;
	}
}

void GameBoss_Update(float delta_time)
{
	for (Boss& b : g_Bosses)
	{
		if (!b.active) { continue; }
		UpdateOne(b, delta_time);
	}
}

// ============================================================================
// Draw
// ============================================================================
static void DrawOne(const Boss& b)
{
	if (b.state == BOSS_TELEGRAPH || b.state == BOSS_AIRBORNE)
	{
		const float total = (b.state == BOSS_TELEGRAPH)
			? TELEGRAPH_TIME + AIRBORNE_TIME
			: AIRBORNE_TIME;
		const float left = (b.state == BOSS_TELEGRAPH)
			? b.state_timer + AIRBORNE_TIME
			: b.state_timer;

		const float t = 1.0f - std::max(0.0f, left / total);

		DrawPrim_Ring(b.jump_to, SLAM_RADIUS, 4.0f, { 0.25f, 0.25f, 0.25f }, 0.85f);
		DrawPrim_Circle(b.jump_to, SLAM_RADIUS * t, { 0.69f, 0.69f, 0.69f }, 0.28f);
	}

	// --- shadow: shrinks as it rises
	if (b.jump_height > 1.0f)
	{
		const float s = 1.0f - (b.jump_height / JUMP_ARC_HEIGHT) * 0.45f;
		DrawPrim_Circle(b.pos, BOSS_RADIUS * s, { 0.0f, 0.0f, 0.0f }, 0.30f);
	}

	// --- the slime ---
	int tex = g_tex_idle[b.variant];
	int frames = IDLE_FRAMES;

	switch (b.state)
	{
	case BOSS_APPROACH:  tex = g_tex_run[b.variant]; frames = RUN_FRAMES;   break;
	case BOSS_TELEGRAPH:
	case BOSS_AIRBORNE:
	case BOSS_LAND:      tex = g_tex_atk[b.variant]; frames = ATK_FRAMES;   break;
	case BOSS_DEAD:      tex = g_tex_death[b.variant]; frames = DEATH_FRAMES; break;
	default: break;
	}

	// death plays ONCE and holds on the last frame; everything else loops
	const int frame = (b.state == BOSS_DEAD)
		? std::min(b.anim_frame, frames - 1)
		: (b.anim_frame % frames);

	const int row = FacingRow(GamePlayer_GetPos() - b.pos);

	SpriteDrawParams p;
	if (b.flash_timer > 0.0f) { p.color = { 2.5f, 2.5f, 2.5f }; }

	Sprite_Draw(
		tex,
		Camera_WorldToScreenX(b.pos.x - BOSS_DRAW_SIZE * 0.5f),
		Camera_WorldToScreenY(b.pos.y - BOSS_DRAW_SIZE * 0.5f - b.jump_height),
		BOSS_DRAW_SIZE, BOSS_DRAW_SIZE,
		static_cast<float>(frame * SHEET_CELL),
		static_cast<float>(row * SHEET_CELL),
		SHEET_CELL, SHEET_CELL,
		p);

	// --- elemental shield ring ---
	if (b.shield_up && b.state != BOSS_DEAD)
	{
		const XMFLOAT3 col = ElementColor(b.shield_element);
		const Vector2  at = b.pos - Vector2{ 0.0f, b.jump_height };

		DrawPrim_Ring(at, BOSS_RADIUS + 18.0f, 4.0f, col, 0.9f);
		DrawPrim_Ring(at, BOSS_RADIUS + 10.0f, 2.0f, col, 0.45f);
	}
}

void GameBoss_Draw()
{
	for (const Boss& b : g_Bosses)
	{
		if (!b.active) { continue; }
		DrawOne(b);
	}
}

// ============================================================================
// UI
// ============================================================================
void GameBoss_DrawUI()
{
	const float w = SCREEN_WIDTH * 0.55f;
	const float x = (SCREEN_WIDTH - w) * 0.5f;
	const float h = 18.0f;

	int drawn = 0;

	for (const Boss& b : g_Bosses)
	{
		if (!b.active) { continue; }

		const float y = 24.0f + drawn * (h + 26.0f);

		char label[32];
		snprintf(label, sizeof(label), "SLIME %d", b.variant + 1);
		GameText_DrawCentered(SCREEN_WIDTH * 0.5f, y - 20.0f, label, 0.45f,
			{ 0.85f, 0.9f, 1.0f });

		// phase colour: the bar says how deep into the fight it is
		XMFLOAT3 fill = { 0.55f, 0.85f, 1.0f };
		if (b.phase == 2) { fill = { 1.00f, 0.75f, 0.30f }; }
		if (b.phase == 3) { fill = { 1.00f, 0.35f, 0.35f }; }

		GameUI_DrawScreenRect(x - 3.0f, y - 3.0f, w + 6.0f, h + 6.0f, { 0.06f, 0.07f, 0.09f }, 0.9f);
		GameUI_DrawScreenRect(x, y, w, h, { 0.18f, 0.12f, 0.14f }, 1.0f);
		GameUI_DrawScreenRect(x, y, w * b.health.Fraction(), h, fill, 1.0f);

		drawn++;
	}
}