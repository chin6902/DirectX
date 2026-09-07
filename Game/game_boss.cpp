/*============================================================================
Contents   :  [game_boss.cpp]

Author     : Chin Qing You
LastUpdate : 2026/09/04
-----------------------------------------------------------------------------
behaviour tag:
	0 BOSS_VARIANT_BASIC  (Slime1) - jump/slam only
	1 BOSS_VARIANT_LASER  (Slime3) - jump/slam + sweeping laser
	2 BOSS_VARIANT_WALL   (Slime2) - jump/slam + wall ring + radial burst
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
#include "sprite.h"
#include "draw_primitives.h"
#include "game_text.h"
#include "game_ui.h"
#include "game_damagenumber.h"
#include "boss_wall.h"
#include "enemy_projectile.h"
#include "game_audio.h"

using namespace DirectX;

static constexpr float BOSS_MAX_HP[BOSS_VARIANT_COUNT] = { 750.0f, 750.0f, 700.0f };

static constexpr float BOSS_RADIUS = 52.0f;
static constexpr float BOSS_DRAW_SIZE = 160.0f;

static constexpr float APPROACH_SPEED = 100.0f;
static constexpr float APPROACH_TIME = 2.20f;
static constexpr float TELEGRAPH_TIME = 1.00f;
static constexpr float AIRBORNE_TIME = 0.75f;
static constexpr float LAND_RECOVER_TIME = 0.90f;
static constexpr float DEATH_TIME = 1.20f;

static constexpr float SLAM_RADIUS = 150.0f;
static constexpr float SLAM_DAMAGE = 4.0f;
static constexpr float SLAM_KNOCKBACK = 520.0f;

static constexpr float JUMP_ARC_HEIGHT = 190.0f;
static constexpr int   JUMPS_PER_SHIELD = 3;

// --- specials ---
static constexpr int   JUMPS_PER_SPECIAL = 2;
static constexpr float SPECIAL_TIME = 2.00f;   

static constexpr float SLAM_DAMAGE_MUL[BOSS_VARIANT_COUNT] = { 1.0f, 0.5f, 0.75f };

// --- laser (BOSS_VARIANT_LASER) ---
static constexpr float LASER_WINDUP = 0.60f;   
static constexpr float LASER_LOCK = 0.30f;  
static constexpr float LASER_SWEEP = 2.40f;
static constexpr float LASER_RANGE = 1400.0f;
static constexpr float LASER_THICKNESS = 27.5f;
static constexpr float LASER_TURN_RATE = 0.35f;   
static constexpr int   LASER_DAMAGE = 2;
static constexpr int   LASER_MOTE_COUNT = 24;
static constexpr float LASER_MOTE_RADIUS = 150.0f;   
static constexpr float LASER_MOTE_STREAK = 18.0f;
static constexpr float LASER_MOTE_CYCLES = 3.0f;
static constexpr XMFLOAT3 LASER_COLOR{ 1.00f, 0.16f, 0.10f };
static constexpr XMFLOAT3 MOTE_COLOR{ 0.47f, 0.08f, 0.08f };

// --- wall (BOSS_VARIANT_WALL) ---
static constexpr float WALL_CAST_TIME = 1.20f;
static constexpr float BURST_INTERVAL = 1.60f;
static constexpr int   BURST_COUNT = 6;
static constexpr float BURST_SPEED = 260.0f;
static constexpr int   BURST_DAMAGE = 2;
static constexpr XMFLOAT3 BURST_COLOR{ 0.85f, 0.45f, 0.00f };

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

static constexpr int   DEATH_GEM_COUNT = 24;
static constexpr int   DEATH_GEM_VALUE = 6;

enum BossState
{
	BOSS_APPROACH,
	BOSS_TELEGRAPH,
	BOSS_AIRBORNE,
	BOSS_LAND,
	BOSS_SPECIAL,
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

	int         special_jumps = 0;   

	bool		laser_firing = false;
	float		laser_angle = 0.0f;

	float       burst_timer = 0.0f;

	int         phase = 1;
	float       summon_timer = 0.0f;
	bool        wall_from_left = true;

	float       anim_timer = 0.0f;
	int         anim_frame = 0;

	BossStatus  status[STATUS_TYPE_COUNT];
};

static Boss g_Bosses[BOSS_MAX]{};

static int g_SpecialTurn = BOSS_VARIANT_LASER;

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

// ============================================================================
// Specials 
// ============================================================================
static float WrapAngle(float a)
{
	while (a > XM_PI) { a -= XM_2PI; }
	while (a < -XM_PI) { a += XM_2PI; }
	return a;
}

static float PointToSegmentDistSq(const Vector2& p, const Vector2& a, const Vector2& b)
{
	const Vector2 ab = b - a;
	const float len_sq = ab.LengthSq();
	if (len_sq <= 0.0001f) { return (p - a).LengthSq(); }

	const float t = std::clamp(Vector2_Dot(p - a, ab) / len_sq, 0.0f, 1.0f);
	return (p - (a + ab * t)).LengthSq();
}

static Vector2 LaserEnd(const Boss& b)
{
	return b.pos + Vector2_FromAngle(b.laser_angle) * LASER_RANGE;
}

static float SpecialDuration(int variant)
{
	switch (variant)
	{
	case BOSS_VARIANT_LASER: return LASER_WINDUP + LASER_LOCK + LASER_SWEEP;
	case BOSS_VARIANT_WALL:  return WALL_CAST_TIME;
	default:                 return SPECIAL_TIME;
	}
}

static void FireRadialBurst(const Boss& b)
{
	const float spin = static_cast<float>(rand() % 628) * 0.01f;

	for (int i = 0; i < BURST_COUNT; i++)
	{
		const float a = spin + (XM_2PI * static_cast<float>(i)) / static_cast<float>(BURST_COUNT);
		EnemyProjectile_Fire(b.pos, Vector2_FromAngle(a), BURST_SPEED, BURST_DAMAGE, BURST_COLOR);
	}

	GameAudio_Play(SND_BOSS_SHOOT);
}

static void BeginSpecial(Boss& b)
{
	if (b.variant == BOSS_VARIANT_LASER)
	{
		b.laser_angle = Vector2_ToAngle(GamePlayer_GetPos() - b.pos);
		b.laser_firing = false;
		GameAudio_Play(SND_BOSS_LASER_CHARGE);
	}

	if (b.variant == BOSS_VARIANT_WALL)
	{
		BossWall_SpawnRing(GamePlayer_GetPos());
		b.burst_timer = BURST_INTERVAL * 0.5f;
		Camera_Shake(14.0f, 0.30f);
		GameAudio_Play(SND_BOSS_WALL);	
	}
}

static void UpdateSpecial(Boss& b, float scaled_dt)
{
	if (b.variant != BOSS_VARIANT_LASER) { return; }

	const Vector2 player = GamePlayer_GetPos();

	// --- windup
	if (b.state_timer > LASER_LOCK + LASER_SWEEP)
	{
		b.laser_angle = Vector2_ToAngle(player - b.pos);
		return;
	}

	// --- lock
	if (b.state_timer > LASER_SWEEP)
	{
		return;
	}

	if (!b.laser_firing)
	{
		b.laser_firing = true;
		GameAudio_Play(SND_BOSS_LASER);
	}

	// --- capped rotation toward the player
	const float want = Vector2_ToAngle(player - b.pos);
	const float delta = WrapAngle(want - b.laser_angle);
	const float step = LASER_TURN_RATE * scaled_dt;

	b.laser_angle += std::clamp(delta, -step, step);
	b.laser_angle = WrapAngle(b.laser_angle);

	const CollisionCircle pc = GamePlayer_GetCollisionCircle();
	const Vector2 pp{ pc.position.x, pc.position.y };

	const float reach = LASER_THICKNESS + pc.radius;
	if (PointToSegmentDistSq(pp, b.pos, LaserEnd(b)) < reach * reach)
	{
		PlayerHit hit;
		hit.damage = LASER_DAMAGE;
		hit.knockback_speed = 260.0f;
		hit.knockback_time = 0.18f;
		hit.direction = Vector2_FromAngle(b.laser_angle);
		GamePlayer_TakeHit(hit);
	}
}

static void EndSpecial(Boss& b)
{
	if (b.variant == BOSS_VARIANT_LASER && b.laser_firing)
	{
		b.laser_firing = false;
		GameAudio_Stop(SND_BOSS_LASER);
	}
}

static void EnterDeath(Boss& b)
{
	EndSpecial(b);           

	if (b.variant == BOSS_VARIANT_WALL) { BossWall_ClearAll(); }

	b.state = BOSS_DEAD;
	b.state_timer = DEATH_TIME;
	b.anim_frame = 0;
	b.anim_timer = 0.0f;
}

// ============================================================================
// Special turn taking
// ============================================================================
static bool SpecialAvailable(const Boss& b)
{
	if (b.variant == BOSS_VARIANT_BASIC) { return false; }

	if (b.variant == BOSS_VARIANT_WALL && BossWall_IsActive()) { return false; }

	return true;
}

static bool AnotherReadyBossAlive(const Boss& self)
{
	for (const Boss& o : g_Bosses)
	{
		if (&o == &self) { continue; }
		if (!o.active || o.state == BOSS_DEAD) { continue; }
		if (!SpecialAvailable(o)) { continue; }
		return true;
	}
	return false;
}

static bool WantsSpecial(const Boss& b)
{
	if (!SpecialAvailable(b)) { return false; }
	if (b.special_jumps < JUMPS_PER_SPECIAL) { return false; }

	if (!AnotherReadyBossAlive(b)) { return true; }

	return g_SpecialTurn == b.variant;
}

void GameBoss_Initialize()
{
	BossVisual_Initialize();

	g_SpecialTurn = BOSS_VARIANT_LASER;

	for (Boss& b : g_Bosses) { b.active = false; }
}

void GameBoss_Finalize()
{
	BossVisual_Finalize();

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

	b.variant = std::clamp(variant - 1, 0, static_cast<int>(BOSS_VARIANT_COUNT) - 1);
	b.summons = summons;

	b.health.Reset(BOSS_MAX_HP[b.variant]);

	b.pos = pos;
	b.jump_from = pos;
	b.jump_to = pos;
	b.jump_height = 0.0f;
	b.flash_timer = 0.0f;

	b.state = BOSS_APPROACH;

	b.state_timer = APPROACH_TIME + STAGGER_PER_INDEX * slot;

	b.special_jumps = 0;

	b.laser_angle = 0.0f;
	b.laser_firing = false;
	b.burst_timer = 0.0f;

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
float   GameBoss_GetRadius(int index) { (void)index; return BOSS_RADIUS; }

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

bool GameBoss_IsGrounded(int index)
{
	if (!ValidIndex(index)) { return false; }
	const Boss& b = g_Bosses[index];
	return b.state != BOSS_AIRBORNE && b.state != BOSS_DEAD;
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

	const float frame_time = BossVisual_GetFrameTime(b.variant, BOSS_CLIP_IDLE);
	b.anim_timer += delta_time;
	if (b.anim_timer >= frame_time)
	{
		b.anim_timer -= frame_time;
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

	if (b.variant == BOSS_VARIANT_WALL && BossWall_IsActive())
	{
		b.special_jumps = 0;

		b.burst_timer -= delta_time * clock;
		if (b.burst_timer <= 0.0f)
		{
			b.burst_timer = BURST_INTERVAL;
			FireRadialBurst(b);
		}
	}

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
				hit.damage = std::max(1, static_cast<int>(SLAM_DAMAGE * SLAM_DAMAGE_MUL[b.variant]));
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

			b.special_jumps++;
		}
		break;
	}

	case BOSS_LAND:
		if (b.state_timer <= 0.0f)
		{
			if (WantsSpecial(b))
			{
				b.special_jumps = 0;
				g_SpecialTurn = (b.variant == BOSS_VARIANT_LASER)
					? BOSS_VARIANT_WALL
					: BOSS_VARIANT_LASER;

				b.state = BOSS_SPECIAL;
				b.state_timer = SpecialDuration(b.variant);
				b.anim_frame = 0;
				BeginSpecial(b);
				break;
			}

			b.state = BOSS_APPROACH;

			b.state_timer = APPROACH_TIME * ((b.phase >= 3) ? 0.6f : (b.phase == 2) ? 0.8f : 1.0f);
			b.anim_frame = 0;
		}
		break;

	case BOSS_SPECIAL:
		UpdateSpecial(b, delta_time * clock);

		if (b.state_timer <= 0.0f)
		{
			EndSpecial(b);

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
static float LaserTwitch(float elapsed)
{
	return 1.0f + 0.14f * sinf(elapsed * 47.0f) + 0.07f * sinf(elapsed * 113.0f);
}

static void DrawLaserCharge(const Boss& b, float charge_t)
{
	charge_t = std::clamp(charge_t, 0.0f, 1.0f);

	const int     index = static_cast<int>(&b - g_Bosses);
	const Vector2 at = b.pos - Vector2{ 0.0f, b.jump_height };

	// cycles bunch toward the end
	const float flow = charge_t * charge_t * LASER_MOTE_CYCLES;

	// cloud tightens as it fills
	const float shell = LASER_MOTE_RADIUS * (1.0f - 0.75f * charge_t * charge_t);

	for (int k = 0; k < LASER_MOTE_COUNT; k++)
	{
		const unsigned int h =
			static_cast<unsigned int>((k + 1) * 73856093) ^
			static_cast<unsigned int>((index + 1) * 19349663);

		const float ang = static_cast<float>(h % 628) * 0.01f;
		const float phase = static_cast<float>((h >> 9) % 100) * 0.01f;
		const float speed = 0.80f + static_cast<float>((h >> 17) % 60) * 0.01f;

		// wrap to 0 -> 1 so motes recycle instead of arriving once
		float p = flow * speed + phase;
		p -= floorf(p);

		// ease-in: they accelerate as they fall inward
		const float e = p * p;
		const float r = shell * (1.0f - e);

		const Vector2 dir = Vector2_FromAngle(ang);
		const Vector2 pos = at + dir * r;
		const Vector2 tail = at + dir * (r + LASER_MOTE_STREAK * (0.30f + e));

		// fade in on birth, out on arrival
		const float fade = sinf(p * XM_PI);
		const float a = fade * (0.30f + 0.70f * charge_t);

		DrawPrim_Line(pos, tail, 2.0f + 2.0f * e, MOTE_COLOR, a * 0.70f);
		DrawPrim_Circle(pos, 1.5f + 2.5f * e, MOTE_COLOR, a);
	}

	// the gathering core, brightening as it fills
	DrawPrim_Circle(at, 5.0f + 18.0f * charge_t, MOTE_COLOR, 0.30f + 0.45f * charge_t);
	DrawPrim_Circle(at, 2.0f + 9.0f * charge_t, { 1.0f, 1.0f, 1.0f }, 0.35f + 0.55f * charge_t);
}

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

	if (b.state == BOSS_SPECIAL && b.variant == BOSS_VARIANT_LASER)
	{
		const float total = LASER_WINDUP + LASER_LOCK + LASER_SWEEP;
		const float elapsed = total - b.state_timer;

		if (b.state_timer > LASER_LOCK + LASER_SWEEP)
		{
			// windup: thin dim tracer
			const float t = elapsed / LASER_WINDUP;
			DrawPrim_BeamGrow(b.pos, LaserEnd(b), LASER_THICKNESS,
				LASER_COLOR, 0.10f, 0.22f + 0.30f * t);
		}
		else if (b.state_timer > LASER_SWEEP)
		{
			const float t = (elapsed - LASER_WINDUP) / LASER_LOCK;
			DrawPrim_BeamGrow(b.pos, LaserEnd(b), LASER_THICKNESS,
				LASER_COLOR, 0.10f + 0.15f * t, 0.55f + 0.45f * t);
		}
		else
		{
			// release shockwave, then the beam
			const float t = 1.0f - (b.state_timer / LASER_SWEEP);

			if (t < 0.12f)
			{
				const float k = t / 0.12f;
				DrawPrim_Ring(b.pos, 24.0f + 190.0f * k, 7.0f * (1.0f - k),
					LASER_COLOR, 1.0f - k);
			}

			const float grow = std::min(1.0f, t / 0.08f);
			const float width = LASER_THICKNESS * LaserTwitch(elapsed);
			DrawPrim_BeamGrow(b.pos, LaserEnd(b), width, LASER_COLOR, grow, 1.0f);
		}

		// charge runs across windup AND lock, collapsing as the beam fires
		if (b.state_timer > LASER_SWEEP)
		{
			const float charge_span = LASER_WINDUP + LASER_LOCK;
			DrawLaserCharge(b, std::min(1.0f, elapsed / charge_span));
		}
	}

	BossVisual_DrawShadow(b.pos, BOSS_RADIUS, b.jump_height, JUMP_ARC_HEIGHT);

	BossClip clip = BOSS_CLIP_IDLE;
	switch (b.state)
	{
	case BOSS_APPROACH:  clip = BOSS_CLIP_RUN;    break;
	case BOSS_TELEGRAPH:
	case BOSS_AIRBORNE:
	case BOSS_LAND:
	case BOSS_SPECIAL:   clip = BOSS_CLIP_ATTACK; break;
	case BOSS_DEAD:      clip = BOSS_CLIP_DEATH;  break;
	default: break;
	}

	SpriteDrawParams p;
	if (b.flash_timer > 0.0f) { p.color = { 2.5f, 2.5f, 2.5f }; }

	BossVisual_DrawBody(
		b.variant, clip,
		b.anim_frame,
		BossVisual_FacingFromDir(GamePlayer_GetPos() - b.pos),
		b.pos, BOSS_DRAW_SIZE, b.jump_height,
		p);

	if (b.shield_up && b.state != BOSS_DEAD)
	{
		BossVisual_DrawShield(b.pos, BOSS_RADIUS, b.jump_height, b.shield_element);
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

		// phase colour
		XMFLOAT3 fill = { 0.55f, 0.85f, 1.0f };
		if (b.phase == 2) { fill = { 1.00f, 0.75f, 0.30f }; }
		if (b.phase == 3) { fill = { 1.00f, 0.35f, 0.35f }; }

		GameUI_DrawScreenRect(x - 3.0f, y - 3.0f, w + 6.0f, h + 6.0f, { 0.06f, 0.07f, 0.09f }, 0.9f);
		GameUI_DrawScreenRect(x, y, w, h, { 0.18f, 0.12f, 0.14f }, 1.0f);
		GameUI_DrawScreenRect(x, y, w * b.health.Fraction(), h, fill, 1.0f);

		drawn++;
	}
}