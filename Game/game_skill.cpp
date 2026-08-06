/*============================================================================
Contents   :  [game_skill.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/06
-----------------------------------------------------------------------------
Two shapes cover the starter skills:
  SHAPE_NOVA       - stationary, radius grows over its lifetime
  SHAPE_PROJECTILE - travels along a velocity, fixed radius

They share one struct and one pool because they differ only by an enum
and whether velocity is zero. Check new skills against this shape before
inventing a second system.

Visuals are debug circles for now: this stage proves the pipeline
(slots -> counts -> lookup -> cast), not the art.
============================================================================*/
#include "game_skill.h"
#include "game_time.h"
#include "camera.h"
#include "collision_debug.h"
#include "element.h"

using namespace DirectX;

// ============================================================================
// Recipe table — THE design document for the combination system.
// Matching is on element COUNTS, so slot order never matters.
// ============================================================================
struct SkillRecipe
{
	int     count[ELEMENT_TYPE_COUNT];   // { fire, ice, thunder }
	SkillId skill;
};

static constexpr SkillRecipe g_Recipes[] =
{
	// --- 1 slot ---
	{ {1,0,0}, SKILL_FIRE_BOLT  },
	{ {0,1,0}, SKILL_ICE_SHARD  },
	// --- 2 slots ---
	{ {2,0,0}, SKILL_FLAME_NOVA },
	// --- 3 slots ---
	{ {3,0,0}, SKILL_INFERNO    },
	// 15 more entries to author: {0,2,0} {0,0,2} {1,1,0} {1,0,1} {0,1,1}
	// {0,3,0} {0,0,3} {2,1,0} {2,0,1} {1,2,0} {0,2,1} {1,0,2} {0,1,2} {1,1,1} {0,0,1}
};

SkillId GameSkill_Lookup(const int* counts)
{
	for (const SkillRecipe& r : g_Recipes)
	{
		if (r.count[ELEMENT_FIRE] == counts[ELEMENT_FIRE]
			&& r.count[ELEMENT_ICE] == counts[ELEMENT_ICE]
			&& r.count[ELEMENT_THUNDER] == counts[ELEMENT_THUNDER])
		{
			return r.skill;
		}
	}
	return SKILL_NONE;
}

// ============================================================================
// Live skill instances
// ============================================================================
enum SkillShape
{
	SHAPE_NOVA,
	SHAPE_PROJECTILE,
};

struct SkillInstance
{
	SkillShape shape;
	Vector2    pos;
	Vector2    vel;
	float      radius;       // nova: current radius; projectile: hit radius
	float      max_radius;   // nova: target radius at end of life
	float      life;         // seconds remaining
	float      max_life;
	XMFLOAT3   color;
	int        power;
	bool       active;
};

static constexpr int SKILL_INSTANCE_MAX = 32;
static SkillInstance g_Instances[SKILL_INSTANCE_MAX]{};

// ----------------------------------------------------------------------------
// Spawn: claim a free pool slot and fill it. Returns the instance so the
// caller can specialise it (see SpawnNova), or nullptr if the pool is full.
// Dropping an effect is always better than overwriting a live one.
// ----------------------------------------------------------------------------
static SkillInstance* Spawn(SkillShape shape, const Vector2& pos, const Vector2& vel,
	float radius, float life, XMFLOAT3 color, int power)
{
	for (SkillInstance& s : g_Instances)
	{
		if (s.active)
		{
			continue;
		}

		s.shape = shape;
		s.pos = pos;
		s.vel = vel;
		s.radius = radius;
		s.max_radius = radius;
		s.life = life;
		s.max_life = life;
		s.color = color;
		s.power = power;
		s.active = true;
		return &s;
	}
	return nullptr;
}

static void SpawnNova(const Vector2& pos, float max_radius, float life,
	XMFLOAT3 color, int power)
{
	// Starts at radius 0 and expands to max_radius over its lifetime.
	if (SkillInstance* s = Spawn(SHAPE_NOVA, pos, { 0.0f, 0.0f }, 0.0f, life, color, power))
	{
		s->max_radius = max_radius;
	}
}

// ============================================================================
// Lifecycle
// ============================================================================
void GameSkill_Initialize()
{
	for (SkillInstance& s : g_Instances)
	{
		s.active = false;
	}
}

void GameSkill_Finalize()
{
	// No resources owned yet (debug-circle visuals). Textures land here later.
}

// ============================================================================
// Cast — one case per authored recipe
// ============================================================================
void GameSkill_Cast(SkillId id, const Vector2& origin, const Vector2& aim, int power)
{
	if (id == SKILL_NONE)
	{
		// Fizzle cue: a small grey puff, so an unauthored combination reads
		// as "that recipe does nothing yet" instead of looking like a bug.
		SpawnNova(origin, 34.0f, 0.18f, { 0.55f, 0.55f, 0.55f }, 0);
		return;
	}

	switch (id)
	{
	case SKILL_FIRE_BOLT:
		Spawn(SHAPE_PROJECTILE, origin, aim * 700.0f,
			18.0f, 1.00f, { 1.00f, 0.45f, 0.10f }, power);
		break;

	case SKILL_ICE_SHARD:
		Spawn(SHAPE_PROJECTILE, origin, aim * 900.0f,
			14.0f, 1.20f, { 0.35f, 0.75f, 1.00f }, power);
		break;

	case SKILL_FLAME_NOVA:
		SpawnNova(origin, 180.0f, 0.35f, { 1.00f, 0.30f, 0.05f }, power);
		break;

	case SKILL_INFERNO:
		SpawnNova(origin, 340.0f, 0.55f, { 1.00f, 0.15f, 0.00f }, power);
		GameTime_Hitstop(0.06f);   // the three-slot cast earns a world-stop
		break;

	default:
		break;
	}
}

// ============================================================================
// Update / Draw
// ============================================================================
void GameSkill_Update(float delta_time)
{
	for (SkillInstance& s : g_Instances)
	{
		if (!s.active)
		{
			continue;
		}

		s.life -= delta_time;
		if (s.life <= 0.0f)
		{
			s.active = false;
			continue;
		}

		if (s.shape == SHAPE_PROJECTILE)
		{
			s.pos += s.vel * delta_time;
			// trail emit point (future): s.pos is the authoritative
			// per-frame position a trail system would subscribe to.

			// LATER: stop on solid tiles via GameStage_IsSolidAtWorld(s.pos.x, s.pos.y)
		}
		else // SHAPE_NOVA
		{
			const float t = 1.0f - (s.life / s.max_life);   // 0 -> 1 over the life
			s.radius = s.max_radius * t;
		}

		// ------------------------------------------------------------------
		// DAMAGE PASS — reattach when the enemy system exists.
		// Same contract player_attack.cpp needs:
		//   count + per-index collision circle + Damage(index, amount) -> killed
		// For every enemy within s.radius of s.pos: Damage(i, s.power).
		// Projectiles should deactivate on their first hit; novas hit all
		// enemies in range once (needs a per-instance "already hit" guard).
		// ------------------------------------------------------------------
	}
}

void GameSkill_Draw()
{
	for (const SkillInstance& s : g_Instances)
	{
		if (!s.active)
		{
			continue;
		}

		Collision_Debug_Draw(
			{ { Camera_WorldToScreenX(s.pos.x), Camera_WorldToScreenY(s.pos.y) },
			  s.radius },
			s.color);
	}
}