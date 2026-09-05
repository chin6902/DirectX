/*============================================================================
Contents   :  [game_skill.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/16
-----------------------------------------------------------------------------
The grid must be built before skills query it(SpatialGrid_Insert)
============================================================================*/
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "game_skill.h"
#include "game_time.h"
#include "game_enemy.h"
#include "game_impact.h"
#include "game_player.h"
#include "game_progress.h"
#include "game_stage.h"
#include "game_ui.h"
#include "camera.h"
#include "config.h"
#include "input_mouse.h"
#include "collision.h"
#include "collision_debug.h"
#include "draw_primitives.h"
#include "game_boss.h"
#include "game_damagenumber.h"
#include "game_item.h"
#include "boss_wall.h"
#include "game_audio.h"

using namespace DirectX;

static constexpr float TWO_PI = 6.2831853f;
static constexpr int BOSS_TARGET_ID = -1;

struct SkillRecipe
{
	int     count[ELEMENT_TYPE_COUNT]; 
	SkillId skill;
};

static constexpr SkillRecipe g_Recipes[] =
{
	// --- 1 slot ---
	{ {1,0,0}, SKILL_FIRE_BOLT        },
	{ {0,1,0}, SKILL_ICE_SHARD        },
	{ {0,0,1}, SKILL_SPARK            },
	// --- 2 slots ---
	{ {2,0,0}, SKILL_FLAME_NOVA       },
	{ {0,2,0}, SKILL_FROST_LANCE      },
	{ {0,0,2}, SKILL_CHAIN_BOLT       },
	{ {1,1,0}, SKILL_STEAM_BURST      },
	{ {1,0,1}, SKILL_OVERLOAD         },
	{ {0,1,1}, SKILL_STATIC_FROST     },
	// --- 3 slots ---
	{ {3,0,0}, SKILL_METEOR           },
	{ {0,3,0}, SKILL_ABSOLUTE_ZERO    },
	{ {0,0,3}, SKILL_TESLA_TURRET     },
	{ {2,1,0}, SKILL_MAGMA_FIELD      },
	{ {1,2,0}, SKILL_FROSTFIRE_SPIKES },
	{ {2,0,1}, SKILL_FIRESTORM        },
	{ {1,0,2}, SKILL_PLASMA_ORBS      },
	{ {0,2,1}, SKILL_GLACIAL_ORBS     },
	{ {0,1,2}, SKILL_STORMFREEZE      },
	{ {1,1,1}, SKILL_PRISM            },
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

static constexpr SoundId g_UltimateVoice[ELEMENT_TYPE_COUNT] =
{
	SND_ULT_FIRE,
	SND_ULT_ICE,
	SND_ULT_ELECTRIC,
};

static SoundId UltimateVoice(ElementType e)
{
	if (e < 0 || e >= ELEMENT_TYPE_COUNT) { return SND_NONE; }
	return g_UltimateVoice[e];
}

enum MotionMode
{
	MOTION_STATIC,         
	MOTION_LINEAR,     
	MOTION_ORBIT,         
	MOTION_FOLLOW_CURSOR,  
	MOTION_AT_CURSOR,       
	MOTION_ORBIT_SEEK,    
	MOTION_BOUNCE,         
};

enum SpawnTrigger
{
	SPAWN_NEVER,
	SPAWN_ON_HIT,      
	SPAWN_ON_TICK,     
	SPAWN_ON_EXPIRE,   
};

enum DrawStyle
{
	DRAW_BEAM,    
	DRAW_DOTS,      
	DRAW_ORB,     
	DRAW_RING,
	DRAW_TESLA,
};

static constexpr float ORBIT_RETURN_TIME = 0.28f;
static constexpr float DIVE_TIMEOUT = 1.20f;

struct MotionSpec
{
	MotionMode mode = MOTION_STATIC;
	float      speed = 0.0f;
	float      orbit_radius = 0.0f;
	float      spin_speed = 0.0f;   
	int        count = 1;      
	float      spread = 0.0f;   
	float      scatter = 0.0f;   
	float      stagger = 0.0f;  
	float      seek_range = 0.0f;   
};

struct HitboxSpec
{
	float radius = 0.0f;
	float half_angle = 0.0f;   
	float length = 0.0f;    
	float thickness = 0.0f;    
	bool  grow = false;  
	bool  screen_wide = false;   
};

struct TargetSpec
{
	float hit_interval = 0.0f;   
	int   max_pierce = 0;
	int   chain_jumps = 0;
	float chain_range = 0.0f;
	float falloff = 1.0f;   
	float min_damage_mul = 1.0f;   
};

struct ImpactSpec
{
	float damage_mul = 1.0f;   // damage = power * damage_mul * falloff
	float kb_speed = 0.0f;
	float kb_time = 0.0f;
	float stun_time = 0.0f;
	float pull_speed = 0.0f;  
};

struct StatusApply
{
	StatusType type = STATUS_NONE;
	float      time = 0.0f;
	float      mag = 0.0f;
};

struct ChildSpec
{
	SkillId      id = SKILL_NONE;
	SpawnTrigger trigger = SPAWN_NEVER;
};

struct SkillDef
{
	ElementType element = ELEMENT_FIRE;  
	MotionSpec  motion;
	float       life = 1.0f;
	float       spawn_delay = 0.0f;   
	HitboxSpec  hitbox;
	TargetSpec  target;
	ImpactSpec  impact;
	StatusApply status[2];
	ChildSpec   child;
	XMFLOAT3    color = { 1.0f, 1.0f, 1.0f };
	float       hitstop = 0.0f;
	DrawStyle   draw_style = DRAW_ORB;
	bool        draw_under = false;
	bool        trail = false;    
	int         max_instances = 0;        

	float       shield_time = 0.0f;
	int         shield_charges = 0;

	float       fall_distance = 0.0f;
	float       fall_angle = 0.0f;
	float       fall_spread = 0.0f;
	float       fall_radius = 22.0f;

	SoundId     sound = SND_NONE;
	float       sound_pitch = 1.0f;
	SoundId     hit_sound = SND_NONE;
	float       hit_pitch = 1.0f;
};

static SkillId      g_PendingId = SKILL_NONE;
static float        g_PendingDelay = 0.0f;
static Vector2      g_PendingOrigin;
static Vector2      g_PendingAim;
static int          g_PendingPower = 0;
static unsigned int g_PendingMask = 0;

static constexpr SkillDef g_SkillDefs[SKILL_ID_COUNT] =
{
	// =========================================================== 1 SLOT
	/* 0 FIRE_BOLT */
	{
		.element = ELEMENT_FIRE,
		.motion = {.mode = MOTION_LINEAR, .speed = 900.0f },
		.life = 0.40f,
		.hitbox = {.radius = 16.0f },
		.impact = {.damage_mul = 1.5f, .kb_speed = 260.0f, .kb_time = 0.15f, .stun_time = 0.10f },
		.color = { 1.00f, 0.45f, 0.10f },
		.trail = true,
		.sound = SND_CAST_FIRE,
		.sound_pitch = 1.15f,
	},

	/* 1 ICE_SHARD  */
	{
		.element = ELEMENT_ICE,
		.motion = {.mode = MOTION_LINEAR, .speed = 520.0f },
		.life = 1.60f,
		.hitbox = {.radius = 14.0f },
		.impact = {.damage_mul = 1.0f, .kb_speed = 100.0f, .kb_time = 0.10f, .stun_time = 0.20f },
		.status = { { STATUS_SLOW, 2.5f, 0.45f } },
		.color = { 0.35f, 0.75f, 1.00f },
		.trail = true,
		.sound = SND_CAST_ICE,
		.sound_pitch = 0.70f,
	},

	/* 2 SPARK */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_LINEAR, .speed = 1100.0f },
		.life = 0.35f,
		.hitbox = {.radius = 12.0f },
		.target = {.chain_jumps = 1, .chain_range = 200.0f, .falloff = 0.5f, .min_damage_mul = 0.1f },
		.impact = {.damage_mul = 1.0f, .kb_speed = 150.0f, .kb_time = 0.10f, .stun_time = 0.15f },
		.color = { 1.00f, 0.90f, 0.30f },
		.trail = true,
		.sound = SND_CAST_THUNDER,
		.sound_pitch = 1.20f,
	},

	// =========================================================== 2 SLOTS
	/* 3 FLAME_NOVA */
	{
		.element = ELEMENT_FIRE,
			.life = 0.35f,
			.hitbox = { .radius = 180.0f, .grow = true },
			.impact = { .damage_mul = 1.3f, .kb_speed = 450.0f, .kb_time = 0.25f, .stun_time = 0.25f },
			.color = { 1.00f, 0.30f, 0.05f },
			.draw_style = DRAW_RING,
			.sound = SND_CAST_FIRE,
			.sound_pitch = 0.90f,
	},

		/* 4 FROST_LANCE */
	{
		.element = ELEMENT_ICE,
		.motion = {.mode = MOTION_LINEAR, .speed = 700.0f },
		.life = 1.20f,
		.hitbox = {.radius = 20.0f },
		.target = {.max_pierce = 8 },
		.impact = {.damage_mul = 0.8f, .kb_speed = 140.0f, .kb_time = 0.12f, .stun_time = 0.20f },
		.status = { { STATUS_SLOW, 3.0f, 0.35f } },
		.color = { 0.55f, 0.85f, 1.00f },
		.trail = true,
		.sound = SND_CAST_ICE,
		.sound_pitch = 1.10f,
	},

	/* 5 CHAIN_BOLT */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_LINEAR, .speed = 950.0f },
		.life = 0.60f,
		.hitbox = {.radius = 16.0f },
		.target = {.chain_jumps = 5, .chain_range = 220.0f, .falloff = 0.5f, .min_damage_mul = 0.1f },
		.impact = {.damage_mul = 1.0f, .kb_speed = 200.0f, .kb_time = 0.12f, .stun_time = 0.18f },
		.color = { 1.00f, 0.95f, 0.45f },
		.trail = true,
		.sound = SND_CAST_THUNDER,
		.sound_pitch = 1.05f,
	},

	/* 6 STEAM_BURST */
	{
		.element = ELEMENT_FIRE,
		.motion = {.mode = MOTION_LINEAR, .speed = 640.0f },
		.life = 0.90f,
		.hitbox = {.radius = 18.0f },
		.impact = {.damage_mul = 1.0f, .kb_speed = 200.0f, .kb_time = 0.12f, .stun_time = 0.15f },
		.child = { SKILL_STEAM_CLOUD, SPAWN_ON_HIT },
		.color = { 0.85f, 0.70f, 0.85f },
		.trail = true,
		.sound = SND_CAST_FIRE,
		.sound_pitch = 0.95f,
	},

	/* 7 OVERLOAD  */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_ORBIT_SEEK, .speed = 720.0f, .orbit_radius = 80.0f, .spin_speed = 2.8f, .count = 1, .seek_range = 300.0f },
		.life = 10.00f,
		.hitbox = {.radius = 20.0f },
		.target = {.hit_interval = 1.50f },
		.impact = {.damage_mul = 1.5f, .kb_speed = 220.0f, .kb_time = 0.12f, .stun_time = 0.20f },
		.status = { { STATUS_BURN, 2.0f, 1.0f } },
		.color = { 1.00f, 0.75f, 0.35f },
		.trail = true,
		.max_instances = 2,
		.sound = SND_SUMMON,
		.sound_pitch = 1.00f,
		.hit_sound = SND_CAST_FIRE,
		.hit_pitch = 0.80f,
	},

	/* 8 STATIC_FROST */
	{
		.element = ELEMENT_ICE,
		.motion = {.mode = MOTION_LINEAR, .speed = 900.0f },
		.life = 0.55f,
		.hitbox = {.radius = 15.0f },
		.target = {.chain_jumps = 3, .chain_range = 210.0f, .falloff = 0.6f, .min_damage_mul = 0.1f },
		.impact = {.damage_mul = 1.0f, .kb_speed = 120.0f, .kb_time = 0.10f, .stun_time = 0.15f },
		.status = { { STATUS_SLOW, 2.5f, 0.5f } },
		.color = { 0.65f, 0.90f, 0.95f },
		.trail = true,
		.sound = SND_CAST_ICE,
		.sound_pitch = 0.90f,
	},

	// =========================================================== 3 SLOTS
	/* 9 METEOR  BURST */
	{
		.element = ELEMENT_FIRE,
		.motion = {.mode = MOTION_AT_CURSOR },
		.life = 1.35f,
		.spawn_delay = 999.0f,
		.hitbox = {.radius = 150.0f },
		.impact = {.damage_mul = 0.0f },
		.child = { SKILL_BIG_METEOR_IMPACT, SPAWN_ON_EXPIRE },
		.color = { 1.00f, 0.35f, 0.05f },
		.fall_distance = 180.0f,
		.fall_angle = -1.05f,
		.fall_radius = 40.0f,
		.sound = SND_CAST_FIRE,
		.sound_pitch = 1.0f,
	},

	/* 10 ABSOLUTE_ZERO */
	{
		.element = ELEMENT_ICE,
		.life = 0.50f,
		.hitbox = {.grow = true, .screen_wide = true },
		.impact = {.damage_mul = 0.5f },
		.status = { { STATUS_FREEZE, 1.0f, 0.0f } },
		.color = { 0.80f, 0.99f, 1.00f },
		.hitstop = 0.08f,
		.draw_style = DRAW_RING,
		.sound = SND_FREEZE,
		.sound_pitch = 0.85f,
	},

	/* 11 TESLA_TURRET */
	{
		.element = ELEMENT_THUNDER,
		.life = 6.00f,
		.hitbox = {.radius = 200.0f },
		.target = {.hit_interval = 0.60f, .chain_jumps = 2, .chain_range = 170.0f, .falloff = 0.4f, .min_damage_mul = 0.1f },
		.impact = {.damage_mul = 0.5f, .kb_speed = 110.0f, .kb_time = 0.08f, .stun_time = 0.10f },
		.color = { 0.85f, 0.85f, 1.00f },
		.draw_style = DRAW_TESLA,
		.max_instances = 2,
		.sound = SND_SUMMON,
		.sound_pitch = 1.00f,
	},

	/* 12 MAGMA_FIELD */
	{
		.element = ELEMENT_FIRE,
		.life = 0.40f,
		.hitbox = {.radius = 80.0f, .grow = true },
		.impact = {.damage_mul = 0.5f, .kb_speed = 380.0f, .kb_time = 0.22f, .stun_time = 0.25f },
		.status = { { STATUS_BURN, 2.0f, 0.5f } },
		.child = { SKILL_MAGMA_GROUND, SPAWN_ON_EXPIRE },
		.color = { 1.00f, 0.35f, 0.05f },
		.hitstop = 0.05f,
		.draw_style = DRAW_RING,
		.sound = SND_CAST_FIRE,
		.sound_pitch = 0.75f,
	},

	/* 13 FROSTFIRE_SPIKES */
	{
		.element = ELEMENT_ICE,
		.motion = {.count = 3, .spread = 0.5236f },   
		.life = 1.00f,
		.spawn_delay = 0.12f,                            
		.hitbox = {.length = 300.0f, .thickness = 26.0f },
		.impact = {.damage_mul = 1.5f, .kb_speed = 260.0f, .kb_time = 0.5f, .stun_time = 0.25f },
		.status = { { STATUS_SLOW, 2.5f, 0.4f } }, 
		.color = { 0.16f, 0.28f, 0.85f },         
		.hitstop = 0.15f,
		.draw_style = DRAW_DOTS,
		.sound = SND_CAST_ICE,
		.sound_pitch = 1.20f,
	},

	/* 14 FIRESTORM */
	{
		.element = ELEMENT_FIRE,
		.motion = {.mode = MOTION_LINEAR, .speed = 520.0f },
		.life = 0.85f,
		.hitbox = {.radius = 20.0f },
		.impact = {.damage_mul = 0.5f, .kb_speed = 150.0f, .kb_time = 0.10f, .stun_time = 0.20f },
		.child = { SKILL_WHIRLPOOL, SPAWN_ON_HIT },
		.color = { 1.00f, 0.55f, 0.15f },
		.trail = true,
		.sound = SND_CAST_FIRE,
		.sound_pitch = 0.85f,
	},

	/* 15 PLASMA_ORBS */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_ORBIT, .orbit_radius = 90.0f, .spin_speed = 2.5f, .count = 3 },
		.life = 7.00f,
		.hitbox = {.radius = 16.0f },
		.target = {.hit_interval = 0.25f },
		.impact = {.damage_mul = 0.3f, .kb_speed = 120.0f, .kb_time = 0.08f, .stun_time = 0.06f },
		.status = { { STATUS_BURN, 2.0f, 0.5f } },
		.color = { 1.00f, 0.65f, 0.35f },
		.trail = true,
		.max_instances = 1,
		.sound = SND_SUMMON,
		.sound_pitch = 1.10f,
		.hit_sound = SND_CAST_FIRE,
		.hit_pitch = 0.70f,
	},

	/* 16 GLACIAL_ORBS */
	{
		.element = ELEMENT_ICE,
		.motion = {.mode = MOTION_ORBIT_SEEK, .speed = 620.0f, .orbit_radius = 85.0f, .spin_speed = 2.2f, .count = 2, .seek_range = 280.0f },
		.life = 7.00f,
		.hitbox = {.radius = 20.0f },
		.target = {.hit_interval = 2.20f },
		.impact = {.damage_mul = 0.3f, .kb_speed = 180.0f, .kb_time = 0.12f, .stun_time = 0.20f },
		.status = { { STATUS_SLOW, 2.0f, 0.6f } },
		.child = { SKILL_FROST_BURST, SPAWN_ON_HIT },
		.color = { 0.60f, 0.90f, 1.00f },
		.trail = true,
		.max_instances = 1,
		.sound = SND_SUMMON,
		.sound_pitch = 0.90f,
		.hit_sound = SND_CAST_ICE,
		.hit_pitch = 0.80f,
	},

	/* 17 STORMFREEZE */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_FOLLOW_CURSOR, .speed = 900.0f, .count = 1 },
		.life = 7.00f,
		.hitbox = {.radius = 30.0f },
		.target = {.hit_interval = 0.25f },
		.impact = {.damage_mul = 0.1f, .kb_speed = 90.0f, .kb_time = 0.08f, .stun_time = 0.10f },
		.status = { { STATUS_SLOW, 1.2f, 0.55f } },
		.color = { 0.70f, 0.85f, 1.00f },
		.trail = true,
		.max_instances = 1,
		.sound = SND_CAST_ICE,
		.sound_pitch = 1.30f,
		.hit_sound = SND_BLIZZARD,
		.hit_pitch = 0.80f,
	},

	/* 18 PRISM  barrier */
	{
		.element = ELEMENT_FIRE,
		.life = 15.00f,
		.hitbox = {.radius = 42.0f }, 
		.impact = {.damage_mul = 0.0f },
		.color = { 0.85f, 0.92f, 1.00f },
		.draw_style = DRAW_RING,
		.shield_time = 10.0f,
		.shield_charges = 1,
		.sound = SND_SUMMON,
		.sound_pitch = 0.70f,
	},

	// =========================================================== ULTIMATES
	/* 19 ULT_FIRE_2  meteor shower */
	{
		.element = ELEMENT_FIRE,
		.motion = {.mode = MOTION_AT_CURSOR, .count = 4, .scatter = 150.0f, .stagger = 0.18f },
		.life = 0.55f,
		.spawn_delay = 0.70f,
		.hitbox = {.radius = 80.0f },
		.impact = {.damage_mul = 0.0f },
		.child = { SKILL_METEOR_IMPACT, SPAWN_ON_EXPIRE },
		.color = { 1.00f, 0.40f, 0.10f },
		.fall_distance = 130.0f,
		.fall_angle = -1.05f,
		.fall_spread = 0.85f,           
		.fall_radius = 17.0f,
		.sound = SND_CAST_FIRE,
		.sound_pitch = 0.90f,
	},

	/* 20 ULT_FIRE_3  meteor storm */
	{
		.element = ELEMENT_FIRE,
		.motion = {.mode = MOTION_AT_CURSOR, .count = 9, .scatter = 420.0f, .stagger = 0.13f },
		.life = 0.70f,
		.spawn_delay = 0.70f,
		.hitbox = {.radius = 80.0f },
		.impact = {.damage_mul = 0.0f },
		.child = { SKILL_METEOR_IMPACT, SPAWN_ON_EXPIRE },
		.color = { 1.00f, 0.30f, 0.02f },
		.hitstop = 0.08f,
		.fall_distance = 340.0f,
		.fall_angle = -1.05f,
		.fall_spread = 1.10f,           
		.fall_radius = 13.0f,
		.sound = SND_CAST_FIRE,
		.sound_pitch = 0.80f,
	},

	/* 21 ULT_ICE_2  frost nova */
	{
		.element = ELEMENT_ICE,
		.life = 0.60f,
		.hitbox = {.radius = 350.0f, .grow = true },
		.impact = {.damage_mul = 0.5f, .kb_speed = 260.0f, .kb_time = 0.20f, .stun_time = 0.30f },
		.status = { { STATUS_SLOW, 7.0f, 0.35f } },
		.color = { 0.65f, 0.92f, 1.00f },
		.hitstop = 0.06f,
		.draw_style = DRAW_RING,
		.sound = SND_CAST_ICE,
		.sound_pitch = 0.90f,
	},

	/* 22 ULT_ICE_3  glacial beam */
	{
		.element = ELEMENT_ICE,
		.life = 0.55f,
		.spawn_delay = 0.25f,
		.hitbox = {.length = 900.0f, .thickness = 30.0f },
		.impact = {.damage_mul = 3.5f, .kb_speed = 200.0f, .kb_time = 0.15f, .stun_time = 0.30f },
		.status = { { STATUS_SLOW, 2.0f, 0.22f } },
		.color = { 0.45f, 0.85f, 1.00f },
		.hitstop = 0.06f,
		.draw_style = DRAW_BEAM,
		.sound = SND_LAZER,
		.sound_pitch = 1.10f,
	},

	/* 23 ULT_THUNDER_2 */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_ORBIT_SEEK, .speed = 780.0f, .orbit_radius = 95.0f, .spin_speed = 3.2f, .count = 2, .seek_range = 240.0f },
		.life = 5.00f,
		.hitbox = {.radius = 22.0f },
		.target = {.hit_interval = 0.50f, .chain_jumps = 2, .chain_range = 190.0f, .falloff = 0.75f, .min_damage_mul = 0.1f },
		.impact = {.damage_mul = 0.2f, .kb_speed = 200.0f, .kb_time = 0.12f, .stun_time = 0.18f },
		.color = { 0.98f, 0.95f, 0.50f },
		.trail = true,
		.max_instances = 1,
		.sound = SND_SUMMON,
		.sound_pitch = 1.00f,
	},

	/* 24 ULT_THUNDER_3  tempest orb */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_BOUNCE, .speed = 620.0f },
		.life = 10.00f,
		.hitbox = {.radius = 42.0f },
		.target = {.hit_interval = 0.22f, .chain_jumps = 3, .chain_range = 230.0f, .falloff = 0.5f, .min_damage_mul = 0.3f },
		.impact = {.damage_mul = 1.0f, .kb_speed = 240.0f, .kb_time = 0.13f, .stun_time = 0.18f },
		.color = { 0.88f, 0.92f, 0.30f },
		.trail = true,
		.max_instances = 3,
		.sound = SND_CAST_THUNDER,
		.sound_pitch = 1.10f,
	},

	// ==================================================== internal children
	/* 25 METEOR_IMPACT */
	{
		.element = ELEMENT_FIRE,
		.life = 0.45f,
		.hitbox = {.radius = 80.0f, .grow = true },
		.impact = {.damage_mul = 2.0f, .kb_speed = 650.0f, .kb_time = 0.30f, .stun_time = 0.35f },
		.status = { { STATUS_BURN, 2.0f, 1.0f } },
		.color = { 1.00f, 0.20f, 0.00f },
		.hitstop = 0.05f,
		.draw_style = DRAW_RING,
		.sound = SND_SMALL_EXPLOSION,
		.sound_pitch = 1.00f,
	},

	/* 26  BIG METEOR_IMPACT */
	{
		.element = ELEMENT_FIRE,
		.life = 0.45f,
		.hitbox = {.radius = 150.0f, .grow = true },
		.impact = {.damage_mul = 2.0f, .kb_speed = 650.0f, .kb_time = 0.30f, .stun_time = 0.35f },
		.status = { { STATUS_BURN, 3.0f, 2.0f } },
		.color = { 1.00f, 0.20f, 0.00f },
		.hitstop = 0.07f,
		.draw_style = DRAW_RING,
		.sound = SND_EXPLOSION,
		.sound_pitch = 0.80f,
	},

	/* 27 FROST_BURST */
	{
		.element = ELEMENT_ICE,
		.life = 0.35f,
		.hitbox = {.radius = 100.0f, .grow = true },
		.impact = {.damage_mul = 0.2f, .kb_speed = 120.0f, .kb_time = 0.10f, .stun_time = 0.10f },
		.status = { { STATUS_SLOW, 2.5f, 0.45f } },
		.color = { 0.55f, 0.88f, 1.00f },
		.draw_style = DRAW_RING,
	},

	/* 28 STEAM_CLOUD */
	{
		.element = ELEMENT_ICE,
		.life = 1.50f,
		.hitbox = {.radius = 80.0f },
		.target = {.hit_interval = 0.80f },
		.impact = {.damage_mul = 0.0f },
		.status = { { STATUS_SLOW, 1.0f, 0.5f }, { STATUS_BURN, 2.0f, 0.5f } },
		.color = { 0.80f, 0.80f, 0.90f },
		.draw_style = DRAW_RING,
	},

	/* 29 MAGMA_GROUND */
	{
		.element = ELEMENT_FIRE,
		.life = 5.00f,
		.hitbox = {.radius = 100.0f },
		.target = {.hit_interval = 0.75f },
		.impact = {.damage_mul = 0.0f },
		.status = { { STATUS_BURN, 3.0f, 0.25f }, { STATUS_SLOW, 3.0f, 0.7f } },
		.color = { 0.65f, 0.05f, 0.00f },
		.draw_style = DRAW_ORB,
		.draw_under = true,
		.max_instances = 2,
	},

	/* 30 WHIRLPOOL */
	{
		.element = ELEMENT_FIRE,
		.life = 3.00f,
		.hitbox = {.radius = 150.0f },
		.target = {.hit_interval = 0.30f },
		.impact = {.damage_mul = 0.0f, .pull_speed = 400.0f },
		.status = { { STATUS_BURN, 0.1f, 0.1f } },
		.color = { 1.00f, 0.45f, 0.10f },
		.draw_style = DRAW_RING,
		.max_instances = 2,
		.sound = SND_WIRLPOOL,
		.sound_pitch = 0.80f,
	},
};

static constexpr int SKILL_INSTANCE_MAX = 96;
static constexpr int SKILL_PIERCE_MAX = 24;   
static constexpr int SKILL_TRAIL_MAX = 64; 

struct SkillInstance
{
	SkillId  id;
	SkillDef def;      
	int      serial;  
	Vector2  pos;
	Vector2  vel;
	Vector2  facing;
	float    life;
	float    max_life;
	float    delay_left;
	float    orbit_angle;
	float    visual_radius;
	float    hit_timer;
	int      power;
	int      target_id;                 
	float    seek_cooldown;
	int      hit_ids[SKILL_PIERCE_MAX];  
	int      hit_count;

	Vector2  trail[SKILL_TRAIL_MAX];
	float    trail_age[SKILL_TRAIL_MAX];
	int      trail_count;
	float    trail_step;      
	float    return_timer;    
	float    return_radius;
	float    dive_timer;
	Vector2  fall_from;

	bool     active;
	unsigned int element_mask;
};

static SkillInstance g_Instances[SKILL_INSTANCE_MAX]{};
static int g_NextSerial = 1;

struct ArcVisual
{
	Vector2  from;
	Vector2  to;
	float    life;
	XMFLOAT3 color;
	bool     active;
};

static constexpr int   ARC_MAX = 64;
static constexpr float ARC_LIFE = 0.15f;
static ArcVisual g_Arcs[ARC_MAX]{};

static void SpawnArc(const Vector2& from, const Vector2& to, const XMFLOAT3& color)
{
	GameAudio_Play(SND_ZAP);

	for (ArcVisual& a : g_Arcs)
	{
		if (a.active) { continue; }
		a.from = from; a.to = to; a.life = ARC_LIFE; a.color = color; a.active = true;
		return;
	}
}

// --- Ice screen overlay -------------------------------------------
static constexpr float FROST_OVERLAY_TIME = 1.50f;   
static float g_FrostTimer = 0.0f;

static constexpr int   FLAKE_MAX = 48;
struct Flake { float x, y, vx, vy, size; };
static Flake g_Flakes[FLAKE_MAX]{};

static void FrostOverlay_Begin()
{
	g_FrostTimer = FROST_OVERLAY_TIME;

	for (Flake& f : g_Flakes)
	{
		f.x = static_cast<float>(rand() % SCREEN_WIDTH);
		f.y = static_cast<float>(rand() % SCREEN_HEIGHT);
		f.vx = -30.0f - static_cast<float>(rand() % 50);   
		f.vy = 20.0f + static_cast<float>(rand() % 40);
		f.size = 2.0f + static_cast<float>(rand() % 3);
	}
}

static Vector2 CursorWorld()
{
	return 
	{
		Camera_ScreenToWorldX(static_cast<float>(InputMouse_GetX())),
		Camera_ScreenToWorldY(static_cast<float>(InputMouse_GetY()))
	};
}

static Vector2 PlayerPos()
{
	return { GamePlayer_GetPosX(), GamePlayer_GetPosY() };
}

static float RandUnit()
{
	return (rand() % 1000) / 1000.0f;
}

static bool AlreadyHit(const SkillInstance& s, int enemy_id)
{
	for (int i = 0; i < s.hit_count; i++)
	{
		if (s.hit_ids[i] == enemy_id) { return true; }
	}
	return false;
}

static void RememberHit(SkillInstance& s, int enemy_id)
{
	if (s.hit_count < SKILL_PIERCE_MAX)
	{
		s.hit_ids[s.hit_count++] = enemy_id;
	}
}

static constexpr float TRAIL_LIFE = 0.18f;
static constexpr float TRAIL_LIFE_ORBIT = 0.40f;
static constexpr float TRAIL_LIFE_BOUNCE = 0.30f;
static constexpr float TRAIL_SECONDS = TRAIL_LIFE;
static constexpr float TRAIL_SECONDS_ORBIT = TRAIL_LIFE_ORBIT;
static constexpr float TRAIL_SECONDS_BOUNCE = TRAIL_LIFE_BOUNCE;

static float TrailLifeFor(const SkillDef& def)
{
	switch(def.motion.mode)
	{
	case MOTION_ORBIT:  return TRAIL_LIFE_ORBIT;
	case MOTION_BOUNCE: return TRAIL_LIFE_BOUNCE;
	default:			return TRAIL_LIFE;
	}
}

static void AgeTrail(SkillInstance& s, const SkillDef& def, float delta_time)
{
	const float life = TrailLifeFor(def);

	int keep = 0;
	for (int i = 0; i < s.trail_count; i++)
	{
		s.trail_age[i] += delta_time;
		if (s.trail_age[i] < life)
		{
			s.trail[keep] = s.trail[i];
			s.trail_age[keep] = s.trail_age[i];
			keep++;
		}
	}
	s.trail_count = keep;
}

static float TrailStepFor(const SkillDef& def)
{
	float speed = def.motion.speed;
	float seconds = TRAIL_SECONDS;

	if (def.motion.mode == MOTION_ORBIT)
	{
		speed = def.motion.orbit_radius * def.motion.spin_speed;
		seconds = TRAIL_SECONDS_ORBIT;
	}
	else if (def.motion.mode == MOTION_BOUNCE)
	{
		seconds = TRAIL_SECONDS_BOUNCE;
	}
	if (speed <= 1.0f) { speed = 200.0f; }

	return std::max(1.0f, speed * seconds / (SKILL_TRAIL_MAX - 1));
}

static bool TrailIsLocal(const SkillDef& def)
{
	return def.motion.mode == MOTION_ORBIT;
}

static void RecordTrail(SkillInstance& s, const SkillDef& def)
{
	if (!def.trail) { return; }

	const Vector2 sample = TrailIsLocal(def) ? (s.pos - PlayerPos()) : s.pos;

	if (s.trail_count > 0)
	{
		const Vector2 last = s.trail[s.trail_count - 1];
		if ((sample - last).LengthSq() < s.trail_step * s.trail_step) { return; }
	}

	if (s.trail_count < SKILL_TRAIL_MAX)
	{
		s.trail[s.trail_count] = sample;
		s.trail_age[s.trail_count] = 0.0f;
		s.trail_count++;
		return;
	}

	for (int i = 0; i < SKILL_TRAIL_MAX - 1; i++)
	{
		s.trail[i] = s.trail[i + 1];
		s.trail_age[i] = s.trail_age[i + 1];
	}
	s.trail[SKILL_TRAIL_MAX - 1] = sample;
	s.trail_age[SKILL_TRAIL_MAX - 1] = 0.0f;
}

static SkillDef ModifiedDef(SkillId id)
{
	SkillDef def = g_SkillDefs[id];
	const ElementMods& m = GameProgress_GetElementMods(def.element);

	def.life *= m.life_mul;
	def.motion.count += m.bonus_count;
	def.impact.damage_mul *= m.damage_mul * GameItem_GetDamageMul();    

	if (def.target.max_pierce > 0)
	{
		def.target.max_pierce += m.bonus_pierce;
	}

	switch (GameProgress_GetUltimateUpgrade())
	{
	case ULT_UPGRADE_ICE:
		for (StatusApply& fx : def.status)
		{
			if (fx.type == STATUS_NONE) { continue; }
			fx.time *= 1.5f;

			fx.mag = (fx.type == STATUS_SLOW) ? (fx.mag * 0.5f) : (fx.mag * 2.0f);
		}
		break;
	case ULT_UPGRADE_THUNDER:
		def.motion.count += 1;
		def.max_instances += (def.max_instances > 0) ? 1 : 0;
		break;
	default:
		break;  
	}

	if (def.target.chain_jumps > 0 && def.element == ELEMENT_THUNDER)
	{
		def.target.chain_jumps += m.bonus_chains;
	}

	for (StatusApply& fx : def.status)
	{
		if (fx.type == STATUS_NONE) { continue; }

		fx.time *= m.status_time;
		fx.mag *= m.status_mag;
	}
	return def;
}

static CollisionCapsule MakeCapsule(const SkillInstance& s, const SkillDef& def)
{
	if (def.motion.mode == MOTION_ORBIT || def.motion.mode == MOTION_ORBIT_SEEK)
	{
		const Vector2 outward = Vector2_FromAngle(s.orbit_angle);
		const Vector2 tangent = { -outward.y, outward.x };     // rotate 90 deg
		const Vector2 half = tangent * (def.hitbox.length * 0.5f);
		return { { s.pos.x - half.x, s.pos.y - half.y },
				 { s.pos.x + half.x, s.pos.y + half.y },
				 def.hitbox.thickness };
	}

	return { { s.pos.x, s.pos.y },
			 { s.pos.x + s.facing.x * def.hitbox.length,
			   s.pos.y + s.facing.y * def.hitbox.length },
			 def.hitbox.thickness };
}

static bool InHitbox(const SkillInstance& s, const SkillDef& def,
	const Vector2& enemy_pos, float enemy_radius)
{
	if (def.hitbox.screen_wide) { return true; }

	if (def.hitbox.length > 0.0f)      // capsule
	{
		const CollisionCircle cc{ { enemy_pos.x, enemy_pos.y }, enemy_radius };
		return Collision_IsOverlap(MakeCapsule(s, def), cc);
	}

	const Vector2 to_e = enemy_pos - s.pos;
	const float reach = def.hitbox.radius + enemy_radius;
	if (to_e.LengthSq() > reach * reach) { return false; }

	if (def.hitbox.half_angle > 0.0f)  // cone
	{
		if (Vector2_Dot(s.facing, Vector2_Normalize(to_e)) < cosf(def.hitbox.half_angle))
		{
			return false;
		}
	}
	return true;
}

static bool DeliverHit(const SkillInstance& s, const SkillDef& def, int enemy_index, float damage, const Vector2& dir)
{
	if (GameEnemy_HasShield(enemy_index) && (s.element_mask & ElementBit(GameEnemy_GetShieldElement(enemy_index))) == 0)
	{
		return false;
	}

	HitInfo hit;
	hit.damage = damage;
	hit.hitstun_time = def.impact.stun_time;
	hit.element_mask = s.element_mask;

	if (def.impact.pull_speed > 0.0f)
	{
		hit.knockback_speed = def.impact.pull_speed;
		hit.knockback_time = def.target.hit_interval * 1.2f;
		hit.direction = -dir;         
	}
	else
	{
		hit.knockback_speed = def.impact.kb_speed;
		hit.knockback_time = def.impact.kb_time;
		hit.direction = dir;
	}

	GameDamageNumber_Spawn(GameEnemy_GetPos(enemy_index), damage, GameEnemy_GetId(enemy_index), def.color);

	const bool killed = GameEnemy_ApplyHit(enemy_index, hit);

	if (!killed)
	{
		for (const StatusApply& fx : def.status)
		{
			if (fx.type == STATUS_NONE) { continue; }
			GameEnemy_ApplyStatus(enemy_index, fx.type, fx.time, fx.mag);
		}
	}

	return killed;
}

void GameSkill_Initialize()
{
	DrawPrim_Initialize();
	for (SkillInstance& s : g_Instances) { s.active = false; }
	for (ArcVisual& a : g_Arcs) { a.active = false; }
	g_NextSerial = 1;
}

void GameSkill_Finalize()
{
	DrawPrim_Finalize();
}

static void InstanceCap(SkillId id, int max_instances, int per_cast)
{
	if (max_instances <= 0) { return; }

	const int allowed = max_instances * std::max(1, per_cast);

	for (;;)
	{
		int count = 0;
		int oldest = -1;

		for (int i = 0; i < SKILL_INSTANCE_MAX; i++)
		{
			if (!g_Instances[i].active || g_Instances[i].id != id) { continue; }

			count++;
			if (oldest < 0 || g_Instances[i].serial < g_Instances[oldest].serial)
			{
				oldest = i;
			}
		}

		if (count + per_cast <= allowed || oldest < 0) { return; }
		g_Instances[oldest].active = false;
	}
}

//==============================================================================================
// Cast
//==============================================================================================
void GameSkill_Cast(SkillId id, const Vector2& origin, const Vector2& aim, int power, unsigned int element_mask)
{
	if (id == SKILL_NONE) { return; }

	const SkillDef def = ModifiedDef(id);         
	const int count = std::max(1, def.motion.count);

	GameAudio_PlayPitched(def.sound, def.sound_pitch);

	if (id == SKILL_ABSOLUTE_ZERO) { FrostOverlay_Begin(); }
	InstanceCap(id, def.max_instances, count);

	int spawned = 0;
	for (SkillInstance& s : g_Instances)
	{
		if (s.active) { continue; }

		Vector2 dir = aim;
		if (count > 1 && def.motion.spread > 0.0f)
		{
			const float offset = def.motion.spread * (spawned - (count - 1) * 0.5f);
			dir = Vector2_FromAngle(Vector2_ToAngle(aim) + offset);
		}

		s.id = id;
		s.def = def;
		s.serial = g_NextSerial++;
		s.facing = dir;
		s.vel = (def.motion.mode == MOTION_LINEAR || def.motion.mode == MOTION_BOUNCE) ? dir * def.motion.speed : Vector2{ 0.0f, 0.0f };

		// --- starting position ---
		switch (def.motion.mode)
		{
		case MOTION_AT_CURSOR:
			s.pos = CursorWorld();
			break;

		case MOTION_ORBIT:
		case MOTION_ORBIT_SEEK:
			s.orbit_angle = TWO_PI * (static_cast<float>(spawned) / count);
			s.pos = PlayerPos() + Vector2_FromAngle(s.orbit_angle) * def.motion.orbit_radius;
			break;

		default:
			s.pos = origin;
			break;
		}

		if (def.motion.scatter > 0.0f)
		{
			s.pos += Vector2_FromAngle(TWO_PI * RandUnit()) * (def.motion.scatter * RandUnit());
		}

		if (def.fall_distance > 0.0f)
		{
			const float angle = def.fall_angle + (RandUnit() * 2.0f - 1.0f) * def.fall_spread;
			const Vector2 dir = Vector2_FromAngle(angle);

			// Distance needed to clear the view along THIS heading. A fixed
			// distance pops in whenever the impact point is near screen
			// centre; deriving it means every meteor enters from offscreen
			// no matter where it lands or which way it comes from.
			const float to_edge_x = (dir.x > 0.0f)
				? (Camera_GetX() + SCREEN_WIDTH - s.pos.x)
				: (s.pos.x - Camera_GetX());
			const float to_edge_y = (dir.y > 0.0f)
				? (Camera_GetY() + SCREEN_HEIGHT - s.pos.y)
				: (s.pos.y - Camera_GetY());

			const float tx = (fabsf(dir.x) > 0.01f) ? to_edge_x / fabsf(dir.x) : 99999.0f;
			const float ty = (fabsf(dir.y) > 0.01f) ? to_edge_y / fabsf(dir.y) : 99999.0f;

			// fall_distance is now a MARGIN past the edge, not the whole span.
			s.fall_from = s.pos + dir * (std::min(tx, ty) + def.fall_distance);
		}
		else
		{
			s.fall_from = s.pos;
		}

		s.life = def.life + def.motion.stagger * spawned;
		s.max_life = s.life;
		s.delay_left = def.spawn_delay + def.motion.stagger * spawned;
		s.visual_radius = def.hitbox.grow ? 0.0f : def.hitbox.radius;
		s.hit_timer = 0.0f;
		s.power = power;
		s.target_id = 0;
		s.seek_cooldown = 0.0f;
		s.hit_count = 0;
		s.trail_count = 0;
		for (float& age : s.trail_age) { age = 0.0f; }
		s.trail_step = TrailStepFor(def);
		s.return_timer = 0.0f;
		s.return_radius = 0.0f;
		s.dive_timer = 0.0f;
		s.active = true;
		s.element_mask = element_mask;

		if (++spawned >= count) { break; }
	}

	if (def.shield_time > 0.0f && def.shield_charges > 0)
	{
		GamePlayer_GrantShield(def.shield_time, def.shield_charges);
	}

	if (def.hitstop > 0.0f) { GameTime_Hitstop(def.hitstop); }
}

static SkillId UltimateFor(ElementType e, int level)
{
	if (level < 2) { return SKILL_NONE; }

	switch (e)
	{
	case ELEMENT_FIRE:    
		return (level >= 3) ? SKILL_ULT_FIRE_3 : SKILL_ULT_FIRE_2;
		break;
	case ELEMENT_ICE:     
		return (level >= 3) ? SKILL_ULT_ICE_3 : SKILL_ULT_ICE_2;
		break;
	case ELEMENT_THUNDER: 
		return (level >= 3) ? SKILL_ULT_THUNDER_3 : SKILL_ULT_THUNDER_2;
		break;
	default:              
		return SKILL_NONE;
		break;
	}
}

void GameSkill_CastUltimate(ElementType e, const Vector2& origin, const Vector2& aim)
{
	SkillId id = UltimateFor(e, GameProgress_GetElementLevel(e));

	if (id == SKILL_NONE)
	{
		int best_level = 1;
		for (int i = 0; i < ELEMENT_TYPE_COUNT; i++)
		{
			const int lv = GameProgress_GetElementLevel(static_cast<ElementType>(i));
			if (lv > best_level) { best_level = lv; e = static_cast<ElementType>(i); }
		}
		id = UltimateFor(e, best_level);
	}

	if (id == SKILL_NONE) { return; }

	GameAudio_Play(UltimateVoice(e));

	GameUI_Flash(g_SkillDefs[id].color, 0.28f);

	const int power = GameProgress_GetElementLevel(e);

	if (GameProgress_GetUltimateUpgrade() == ULT_UPGRADE_FIRE)
	{
		const int split = std::max(1, static_cast<int>(power * 0.75f));
		if (g_SkillDefs[id].max_instances < 1)
		{
			GameSkill_Cast(id, origin, aim, split, ElementBit(e));
		}
		else
		{
			GameSkill_Cast(id, origin, aim, power, ElementBit(e));
		}
		
		g_PendingId = id;
		g_PendingDelay = 0.9f;
		g_PendingOrigin = origin;
		g_PendingAim = aim;
		g_PendingPower = split;
		g_PendingMask = ElementBit(e);
		return;
	}

	GameSkill_Cast(id, origin, aim, power, ElementBit(e));
}

// ============================================================================
// Damage
// ============================================================================
static void TryHitWall(SkillInstance& s, const SkillDef& def)
{
	const int count = BossWall_GetSegmentCount();

	for (int i = 0; i < count; i++)
	{
		if (!BossWall_IsSegmentAlive(i)) { continue; }

		const int wall_hit_id = -200 - i;
		if (AlreadyHit(s, wall_hit_id)) { continue; }

		const Vector2 wp = BossWall_GetSegmentPos(i);
		if (!InHitbox(s, def, wp, BossWall_GetSegmentRadius(i))) { continue; }

		RememberHit(s, wall_hit_id);

		HitInfo hit;
		hit.damage = 0.0f;
		hit.element_mask = s.element_mask;
		hit.hitstun_time = 0.0f;
		hit.direction = Vector2_Normalize(wp - s.pos);

		if (BossWall_ApplyHit(i, hit))
		{
			GameAudio_PlayPitched(def.hit_sound, def.hit_pitch);
		}
	}
}

static bool TryHitBoss(SkillInstance& s, const SkillDef& def)
{
	bool hit_any = false;

	for (int i = 0; i < BOSS_MAX; i++)
	{
		if (!GameBoss_IsActive(i)) { continue; }

		const int boss_hit_id = -100 - i;       
		if (AlreadyHit(s, boss_hit_id)) { continue; }

		const Vector2 boss_pos = GameBoss_GetPos(i);
		if (!InHitbox(s, def, boss_pos, GameBoss_GetRadius(i))) { continue; }

		if (GameBoss_HasShield(i) && (s.element_mask & ElementBit(GameBoss_GetShieldElement(i))) == 0)
		{
			continue;
		}

		RememberHit(s, boss_hit_id);

		if (!hit_any) { GameAudio_PlayPitched(def.hit_sound, def.hit_pitch); }

		const Vector2 dir = Vector2_Normalize(boss_pos - s.pos);

		HitInfo hit;
		hit.damage = std::max(0.5f, s.power * def.impact.damage_mul);
		hit.element_mask = s.element_mask;
		hit.hitstun_time = 0.0f;      
		hit.direction = dir;

		GameBoss_ApplyHit(i, hit);

		GameDamageNumber_Spawn(boss_pos, hit.damage, -100 - i, def.color);

		for (const StatusApply& fx : def.status)
		{
			if (fx.type == STATUS_NONE) { continue; }
			GameBoss_ApplyStatus(i, fx.type, fx.time, fx.mag);
		}

		if (def.child.id != SKILL_NONE && (def.child.trigger == SPAWN_ON_HIT || def.child.trigger == SPAWN_ON_TICK))
		{
			GameSkill_Cast(def.child.id, boss_pos, dir, s.power, s.element_mask);
		}

		if (def.target.chain_jumps > 0) { SpawnArc(s.pos, boss_pos, def.color); }

		hit_any = true;
	}

	return hit_any;
}

static constexpr int SKILL_QUERY_MAX = 128;
static constexpr float MAX_ENEMY_RADIUS = 34.0f;

static float QueryReach(const SkillDef& def)
{
	if (def.hitbox.length > 0.0f) 
	{ 
		return def.hitbox.length + def.hitbox.thickness + MAX_ENEMY_RADIUS;
	}

	return def.hitbox.radius + MAX_ENEMY_RADIUS;
}

static void DamagePass(SkillInstance& s, const SkillDef& def)
{
	TryHitWall(s, def);
	const bool hit_boss = TryHitBoss(s, def);
	if (hit_boss && def.motion.mode == MOTION_LINEAR && def.target.hit_interval <= 0.0f && s.hit_count > def.target.max_pierce)
	{
		s.active = false;
		return;
	}

	int targets_hit = 0;

	int candidates[SKILL_QUERY_MAX]{};
	int candidate_count = 0;
	int hop[SKILL_QUERY_MAX]{};

	if (def.hitbox.screen_wide)
	{
		constexpr float MARGIN = 64.0f;
		const float min_x = Camera_GetX() - MARGIN;
		const float min_y = Camera_GetY() - MARGIN;
		const float max_x = Camera_GetX() + SCREEN_WIDTH + MARGIN;
		const float max_y = Camera_GetY() + SCREEN_HEIGHT + MARGIN;

		const int active = GameEnemy_GetActiveCount();
		for (int k = 0; k < active && candidate_count < SKILL_QUERY_MAX; k++)
		{
			const CollisionCircle cc = GameEnemy_GetCollisionCircle(k);
			if (cc.position.x < min_x || cc.position.x > max_x) { continue; }
			if (cc.position.y < min_y || cc.position.y > max_y) { continue; }
			candidates[candidate_count++] = k;
		}
	}
	else
	{
		candidate_count = GameEnemy_QueryRadius(s.pos, QueryReach(def), candidates, SKILL_QUERY_MAX);
	}

	for (int k = 0; k < candidate_count; k++)
	{
		const int i = candidates[k];
		const int enemy_id = GameEnemy_GetId(i);
		if (AlreadyHit(s, enemy_id)) { continue; }

		const CollisionCircle cc = GameEnemy_GetCollisionCircle(i);
		const Vector2 enemy_pos = { cc.position.x, cc.position.y };
		if (!InHitbox(s, def, enemy_pos, cc.radius)) { continue; }

		if (targets_hit == 0)
		{
			GameAudio_PlayPitched(def.hit_sound, def.hit_pitch);
		}

		float mul = 1.0f;
		for (int f = 0; f < targets_hit; f++) { mul *= def.target.falloff; }
		mul = std::max(mul, def.target.min_damage_mul);

		const float damage = std::max(0.5f, s.power * def.impact.damage_mul * mul);
		const Vector2 offset = enemy_pos - s.pos;
		const Vector2 dir = (offset.LengthSq() < 0.01f) ? s.facing : Vector2_Normalize(offset);

		DeliverHit(s, def, i, damage, dir);
		RememberHit(s, enemy_id);
		targets_hit++;

		// child spawned at the target
		if (def.child.id != SKILL_NONE && (def.child.trigger == SPAWN_ON_HIT || def.child.trigger == SPAWN_ON_TICK))
		{
			GameSkill_Cast(def.child.id, enemy_pos, dir, s.power, s.element_mask);

			if (GameProgress_GetUltimateUpgrade() == ULT_UPGRADE_THUNDER)
			{
				GameSkill_Cast(def.child.id, enemy_pos, dir, s.power, s.element_mask);
			}
		}

		if (def.target.chain_jumps > 0) { SpawnArc(s.pos, enemy_pos, def.color); }

		// chain: each hop searches from the last enemy hit, walking outward
		Vector2 arc_from = enemy_pos;
		for (int jump = 0; jump < def.target.chain_jumps; jump++)
		{
			int best = -1;
			float best_sq = def.target.chain_range * def.target.chain_range;

			const int hop_count = GameEnemy_QueryRadius(arc_from, def.target.chain_range + MAX_ENEMY_RADIUS, hop, SKILL_QUERY_MAX);

			for (int h = 0; h < hop_count; h++)
			{
				const int j = hop[h];
				if (AlreadyHit(s, GameEnemy_GetId(j))) { continue; }
				const float d_sq = (GameEnemy_GetPos(j) - arc_from).LengthSq();
				if (d_sq < best_sq) { best_sq = d_sq; best = j; }
			}
			if (best < 0) { break; }   // nothing left in range: chain ends

			float arc_mul = 1.0f;
			for (int f = 0; f < targets_hit; f++) { arc_mul *= def.target.falloff; }
			arc_mul = std::max(arc_mul, def.target.min_damage_mul);

			const Vector2 arc_pos = GameEnemy_GetPos(best);
			const Vector2 arc_dir = Vector2_Normalize(arc_pos - arc_from);

			DeliverHit(s, def, best, std::max(0.5f, s.power * def.impact.damage_mul * arc_mul), arc_dir);
			SpawnArc(arc_from, arc_pos, def.color);
			RememberHit(s, GameEnemy_GetId(best));

			targets_hit++;
			arc_from = arc_pos;
		}

		if (def.motion.mode == MOTION_ORBIT_SEEK) { return; }

		if (def.motion.mode == MOTION_LINEAR && def.target.hit_interval <= 0.0f && s.hit_count > def.target.max_pierce)
		{
			s.active = false;
			return;
		}
	}
}

void GameSkill_Update(float delta_time)
{
	if (g_PendingId != SKILL_NONE)
	{
		g_PendingDelay -= delta_time;
		if (g_PendingDelay <= 0.0f)
		{
			const SkillId id = g_PendingId;
			g_PendingId = SKILL_NONE;  

			GameUI_Flash(g_SkillDefs[id].color, 0.28f);
			GameSkill_Cast(id, g_PendingOrigin, g_PendingAim, g_PendingPower, g_PendingMask);
		}
	}

	for (ArcVisual& a : g_Arcs)
	{
		if (!a.active) { continue; }
		a.life -= delta_time;
		if (a.life <= 0.0f) { a.active = false; }
	}

	if (g_FrostTimer > 0.0f)
	{
		g_FrostTimer -= delta_time;
		for (Flake& f : g_Flakes)
		{
			f.x += f.vx * delta_time;
			f.y += f.vy * delta_time;
			if (f.y > SCREEN_HEIGHT) { f.y -= SCREEN_HEIGHT; }
			if (f.x < 0.0f) { f.x += SCREEN_WIDTH; }
		}
	}

	for (SkillInstance& s : g_Instances)
	{
		if (!s.active) { continue; }

		const SkillDef& def = s.def;

		s.life -= delta_time;
		if (s.life <= 0.0f)
		{
			s.active = false;
			if (def.child.id != SKILL_NONE && def.child.trigger == SPAWN_ON_EXPIRE)
			{
				GameSkill_Cast(def.child.id, s.pos, s.facing, s.power, s.element_mask);
			}
			continue;
		}

		if (def.trail) { AgeTrail(s, def, delta_time); }

		// --- position for this frame ---
		switch (def.motion.mode)
		{
		case MOTION_LINEAR:
			s.pos += s.vel * delta_time;
			RecordTrail(s, def);
			break;

		case MOTION_ORBIT:
			s.orbit_angle += def.motion.spin_speed * delta_time;
			s.pos = PlayerPos() + Vector2_FromAngle(s.orbit_angle) * def.motion.orbit_radius;
			RecordTrail(s, def);
			break;

		case MOTION_FOLLOW_CURSOR:
		{
			const Vector2 to_cursor = CursorWorld() - s.pos;
			if (to_cursor.LengthSq() > 1.0f)
			{
				s.pos += Vector2_Normalize(to_cursor) * (def.motion.speed * delta_time);
			}
			RecordTrail(s, def);
			break;
		}

		case MOTION_ORBIT_SEEK:
		{
			if (s.seek_cooldown > 0.0f) { s.seek_cooldown -= delta_time; }

			if (s.target_id == 0)
			{
				// no target: orbit around the player
				s.orbit_angle += def.motion.spin_speed * delta_time;

				float radius = def.motion.orbit_radius;
				if (s.return_timer > 0.0f)
				{
					s.return_timer -= delta_time;
					const float t = std::clamp(1.0f - s.return_timer / ORBIT_RETURN_TIME, 0.0f, 1.0f);
					const float e = 1.0f - (1.0f - t) * (1.0f - t);   // ease out
					radius = s.return_radius + (def.motion.orbit_radius - s.return_radius) * e;
				}

				s.pos = PlayerPos() + Vector2_FromAngle(s.orbit_angle) * radius;


				if (s.seek_cooldown <= 0.0f && s.hit_timer <= 0.0f)
				{
					const float seek_sq = def.motion.seek_range * def.motion.seek_range;

					for (int i = 0; i < BOSS_MAX; i++)
					{
						if (!GameBoss_IsActive(i)) { continue; }
						if ((GameBoss_GetPos(i) - s.pos).LengthSq() >= seek_sq) { continue; }
						if (GameBoss_HasShield(i) && (s.element_mask & ElementBit(GameBoss_GetShieldElement(i))) == 0) { continue; }
						
						s.target_id = BOSS_TARGET_ID - i;
						break;
					}

					if (s.target_id == 0)       
					{
						for (int i = 0; i < GameEnemy_GetActiveCount(); i++)
						{
							if ((GameEnemy_GetPos(i) - s.pos).LengthSq() >= seek_sq) { continue; }

							s.target_id = GameEnemy_GetId(i);
							break;
						}
					}
				}
			}
			else
			{
				// has a target: move toward it
				s.dive_timer += delta_time;

				if (s.dive_timer >= DIVE_TIMEOUT) 
				{
					const Vector2 from_player = s.pos - PlayerPos();
					s.orbit_angle = Vector2_ToAngle(from_player);
					s.return_radius = from_player.Length();
					s.return_timer = ORBIT_RETURN_TIME;
					s.target_id = 0;
					s.seek_cooldown = 0.3f;
					s.dive_timer = 0.0f;
					RecordTrail(s, def);
					break;
				}

				Vector2 target_pos;
				if (s.target_id < 0)       
				{
					const int boss_idx = BOSS_TARGET_ID - s.target_id;   
					if (boss_idx < 0 || boss_idx >= BOSS_MAX || !GameBoss_IsActive(boss_idx))
					{
						s.target_id = 0;
						break;
					}
					target_pos = GameBoss_GetPos(boss_idx);
				}
				else
				{
					const int idx = GameEnemy_FindById(s.target_id);
					if (idx < 0) { s.target_id = 0; break; }
					target_pos = GameEnemy_GetPos(idx);
				}

				const Vector2 to_target = target_pos - s.pos;
				if (to_target.LengthSq() > 1.0f)
				{
					s.facing = Vector2_Normalize(to_target);
					s.pos += s.facing * (def.motion.speed * delta_time);
				}
			}
			RecordTrail(s, def);
			break;
		}

		case MOTION_BOUNCE:
		{
			const float left = Camera_GetX() + def.hitbox.radius;
			const float top = Camera_GetY() + def.hitbox.radius;
			const float right = Camera_GetX() + SCREEN_WIDTH - def.hitbox.radius;
			const float bottom = Camera_GetY() + SCREEN_HEIGHT - def.hitbox.radius;

			s.pos += s.vel * delta_time;

			if (s.pos.x < left) { s.pos.x = left;   s.vel.x = -s.vel.x; }
			if (s.pos.x > right) { s.pos.x = right;  s.vel.x = -s.vel.x; }
			if (s.pos.y < top) { s.pos.y = top;    s.vel.y = -s.vel.y; }
			if (s.pos.y > bottom) { s.pos.y = bottom; s.vel.y = -s.vel.y; }

			if (!s.vel.IsZero()) { s.facing = Vector2_Normalize(s.vel); }
			RecordTrail(s, def);
			break;
		}

		case MOTION_STATIC:
			if (def.shield_charges > 0) { s.pos = PlayerPos(); }
			break;

		default:  
			break;
		}

		s.visual_radius = def.hitbox.grow ? def.hitbox.radius * (1.0f - (s.life / s.max_life)) : def.hitbox.radius;

		if (s.delay_left > 0.0f)
		{
			s.delay_left -= delta_time;
			continue;
		}

		if (def.shield_charges > 0)
		{
			if (!GamePlayer_IsShielded()) { s.active = false; }
			continue;
		}

		if (def.impact.damage_mul <= 0.0f && def.status[0].type == STATUS_NONE && def.child.id == SKILL_NONE)
		{
			continue;
		}

		if (def.motion.mode == MOTION_ORBIT_SEEK)
		{
			if (s.hit_timer > 0.0f) { s.hit_timer -= delta_time; }

			if (s.target_id != 0 && s.hit_timer <= 0.0f)
			{
				s.hit_count = 0;
				DamagePass(s, def);

				if (s.hit_count > 0)   
				{
					const Vector2 from_player = s.pos - PlayerPos();
					s.orbit_angle = Vector2_ToAngle(from_player);
					s.return_radius = from_player.Length();    
					s.return_timer = ORBIT_RETURN_TIME;
					s.target_id = 0;
					s.seek_cooldown = 0.3f;
					s.hit_timer = def.target.hit_interval;
					s.dive_timer = 0.0f;
				}
			}
			continue;
		}

		// --- damage timing ---
		if (def.target.hit_interval > 0.0f)
		{
			s.hit_timer -= delta_time;
			if (s.hit_timer <= 0.0f)
			{
				s.hit_timer = def.target.hit_interval;
				s.hit_count = 0;
				DamagePass(s, def);
			}
		}
		else if (s.hit_count == 0 || def.target.max_pierce > 0)
		{
			DamagePass(s, def);
		}
	}
}

// ============================================================================
// Draw
// ============================================================================
static void DrawInstances(bool under_layer)
{
	for (const SkillInstance& s : g_Instances)
	{
		if (!s.active) { continue; }
		if (s.def.draw_under != under_layer) { continue; }

		const SkillDef& def = s.def;

		// --- screenwide ring out from the player ---
		if (def.hitbox.screen_wide)
		{
			const float r = 400.0f * (1.0f - (s.life / s.max_life));
			DrawPrim_Ring(PlayerPos(), r, 7.0f, def.color, 0.85f);
			DrawPrim_Ring(PlayerPos(), r * 0.82f, 3.0f, def.color, 0.45f);
			continue;
		}

		// --- telegraph: where it will land, filled disc ---
		if (s.delay_left > 0.0f && def.spawn_delay > 0.0f)
		{
			if (def.hitbox.length > 0.0f) { continue; }

			const float life_t = 1.0f - (s.life / s.max_life);
			const float r = (def.hitbox.radius > 0.0f) ? def.hitbox.radius : def.hitbox.length * 0.5f;

			DrawPrim_Ring(s.pos, r, 3.0f, def.color, 0.9f);
			DrawPrim_Circle(s.pos, r * life_t, def.color, 0.30f);

			if (def.fall_distance > 0.0f)
			{
				const float fall_t = life_t * life_t;      // accelerating
				const Vector2 at = Vector2_Lerp(s.fall_from, s.pos, fall_t);
				const Vector2 dir = Vector2_Normalize(s.pos - s.fall_from);
				const float   rad = def.fall_radius;

				DrawPrim_Beam(at - dir * (rad * 6.8f), at, rad * 0.75f, def.color, 0.55f);
				DrawPrim_Beam(at - dir * (rad * 3.2f), at, rad * 1.05f, def.color, 0.85f);

				DrawPrim_Circle(at, rad * 1.25f, { 1.0f, 0.62f, 0.15f }, 0.9f);
				DrawPrim_Circle(at, rad, { 0.22f, 0.13f, 0.11f }, 1.0f);
				DrawPrim_Circle(at - dir * (rad * 0.35f), rad * 0.45f,
					{ 1.0f, 0.85f, 0.45f }, 0.8f);
			}
			continue;
		}

		// --- capsule ---
		if (def.hitbox.length > 0.0f)
		{
			const CollisionCapsule cap = MakeCapsule(s, def);
			const Vector2 a{ cap.start.x, cap.start.y };
			const Vector2 b{ cap.end.x,   cap.end.y };

			if (def.draw_style == DRAW_DOTS)
			{
				constexpr int SPIKES = 7;
				for (int k = 0; k <= SPIKES; k++)
				{
					const float t = static_cast<float>(k) / SPIKES;
					const Vector2 p = Vector2_Lerp(a, b, t);
					const float r = def.hitbox.thickness * (0.55f + 0.45f * t);

					DrawPrim_Circle(p, r, def.color, 0.95f);
					DrawPrim_Circle(p, r * 0.42f, { 0.62f, 0.82f, 1.0f }, 0.9f);   // blue core
					DrawPrim_Ring(p, r * 1.25f, 2.0f, def.color, 0.4f);
				}
			}
			else
			{
				const float vis = std::max(0.0001f, s.max_life - def.spawn_delay);
				const float beam_t = ((s.max_life - s.life) - def.spawn_delay) / vis;
				const float grow = std::min(1.0f, beam_t / 0.35f);                       
				const float fade = (beam_t <= 0.70f) ? 1.0f : 1.0f - (beam_t - 0.70f) / 0.30f;
				DrawPrim_BeamGrow(a, b, def.hitbox.thickness, def.color, grow, fade);
			}

#ifdef _DEBUG
			for (int k = 0; k <= 8; k++)
			{
				const Vector2 p = Vector2_Lerp(a, b, static_cast<float>(k) / 8.0f);
				Collision_Debug_Draw(
					{ { Camera_WorldToScreenX(p.x), Camera_WorldToScreenY(p.y) },
					  def.hitbox.thickness },
					{ 0.35f, 0.35f, 0.35f });
			}
#endif
			continue;
		}

		// --- cone ---
		if (def.hitbox.half_angle > 0.0f)
		{
			const float base = Vector2_ToAngle(s.facing);
			const float reach = s.visual_radius;

			constexpr int SLICES = 9;
			for (int k = 0; k < SLICES; k++)
			{
				const float a = base - def.hitbox.half_angle
					+ (def.hitbox.half_angle * 2.0f) * ((k + 0.5f) / SLICES);
				DrawPrim_Line(s.pos, s.pos + Vector2_FromAngle(a) * reach,
					reach * 0.10f, def.color, 0.45f);
			}
			continue;
		}

		// --- trail ---
		if (def.trail && s.trail_count >= 2)
		{
			const Vector2* pts = s.trail;

			Vector2 world[SKILL_TRAIL_MAX];
			if (TrailIsLocal(def))
			{
				const Vector2 p = PlayerPos();
				for (int i = 0; i < s.trail_count; i++)
				{
					world[i] = s.trail[i] + p;
				}
				pts = world;
			}

			DrawPrim_Trail(pts, s.trail_count,
				def.hitbox.radius * 0.8f, def.color, 0.8f, 0.0f);
		}

		// --- circle ---
		if (def.draw_style == DRAW_RING)
		{
			DrawPrim_Ring(s.pos, s.visual_radius, 3.0f, def.color, 0.85f);

			if (def.element == ELEMENT_ICE && def.hitbox.radius > 300.0f)
			{
				const float t = 1.0f - (s.life / s.max_life);
				const float haze = (1.0f - t) * 0.45f;

				constexpr int MOTES = 22;
				for (int k = 0; k < MOTES; k++)
				{
					// fixed per-instance scatter: serial keeps it stable frame to frame
					const unsigned int h = static_cast<unsigned int>(s.serial * 73856093 + k * 19349663);
					const float ang = (h % 628) * 0.01f;
					const float rad = s.visual_radius * (0.15f + 0.85f * ((h >> 9) % 100) * 0.01f);
					const float sz = 1.5f + ((h >> 17) % 3);

					const Vector2 p{ s.pos.x + cosf(ang) * rad,
									 s.pos.y + sinf(ang) * rad };

					DrawPrim_Circle(p, sz, { 1.0f, 1.0f, 1.0f }, haze);
				}
			}
		}
		else if (def.draw_style == DRAW_TESLA)
		{
			float tesla_radius = s.visual_radius * 0.3f;

			DrawPrim_Circle(s.pos, tesla_radius, def.color, 0.85f);
		}
		else
		{
			DrawPrim_Circle(s.pos, s.visual_radius, def.color, 0.85f);
			DrawPrim_Ring(s.pos, s.visual_radius * 1.35f, 2.0f, def.color, 0.5f);
		}

#ifdef _DEBUG
		Collision_Debug_Draw(
			{ { Camera_WorldToScreenX(s.pos.x), Camera_WorldToScreenY(s.pos.y) },
			  def.hitbox.radius },
			{ 0.35f, 0.35f, 0.35f });
#endif
	}
}

void GameSkill_DrawUnder()
{
	DrawInstances(true);
}

void GameSkill_Draw()
{
	for (const ArcVisual& a : g_Arcs)
	{
		if (!a.active) { continue; }

		const float fade = a.life / ARC_LIFE;
		DrawPrim_Beam(a.from, a.to, 2.0f + 3.0f * fade, a.color, fade);
	}

	DrawInstances(false);
}

void GameSkill_DrawOverlay()
{
	if (g_FrostTimer <= 0.0f) { return; }

	// ease in fast, fade out over the tail
	const float t = g_FrostTimer / FROST_OVERLAY_TIME;
	const float in = std::min(1.0f, (1.0f - t) / 0.12f);
	const float a = in * std::min(1.0f, t / 0.45f);

	// --- gradient border
	constexpr int BANDS = 10;
	constexpr float DEPTH = 130.0f;
	for (int i = 0; i < BANDS; i++)
	{
		const float f = static_cast<float>(i) / BANDS;
		const float inset = DEPTH * f;
		const float band = DEPTH / BANDS + 1.0f;
		const float ba = a * 0.16f * (1.0f - f);

		// blue at the edge -> white as it moves inward
		const XMFLOAT3 col{ 0.55f + 0.45f * f, 0.80f + 0.20f * f, 1.0f };

		GameUI_DrawScreenRect(inset, inset, SCREEN_WIDTH - inset * 2.0f, band, col, ba);
		GameUI_DrawScreenRect(inset, SCREEN_HEIGHT - inset - band,
			SCREEN_WIDTH - inset * 2.0f, band, col, ba);
		GameUI_DrawScreenRect(inset, inset, band, SCREEN_HEIGHT - inset * 2.0f, col, ba);
		GameUI_DrawScreenRect(SCREEN_WIDTH - inset - band, inset,
			band, SCREEN_HEIGHT - inset * 2.0f, col, ba);
	}

	// --- snowflakes
	for (const Flake& f : g_Flakes)
	{
		GameUI_DrawScreenRect(f.x, f.y, f.size, f.size, { 1.0f, 1.0f, 1.0f }, a * 0.85f);
	}
}