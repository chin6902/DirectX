/*============================================================================
Contents   :  [game_skill.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/16
-----------------------------------------------------------------------------

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

using namespace DirectX;

static constexpr float TWO_PI = 6.2831853f;

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
	bool        trail = false;    
	int         max_instances = 0;        

	float       shield_time = 0.0f;
	int         shield_charges = 0;
};

static constexpr SkillDef g_SkillDefs[SKILL_ID_COUNT] =
{
	// =========================================================== 1 SLOT
	/* 0 FIRE_BOLT */
	{
		.element = ELEMENT_FIRE,
		.motion = {.mode = MOTION_LINEAR, .speed = 900.0f },
		.life = 0.40f,
		.hitbox = {.radius = 16.0f },
		.impact = {.damage_mul = 1.0f, .kb_speed = 260.0f, .kb_time = 0.15f, .stun_time = 0.10f },
		.color = { 1.00f, 0.45f, 0.10f },
		.trail = true,
	},

	/* 1 ICE_SHARD  */
	{
		.element = ELEMENT_ICE,
		.motion = {.mode = MOTION_LINEAR, .speed = 520.0f },
		.life = 1.60f,
		.hitbox = {.radius = 14.0f },
		.impact = {.damage_mul = 0.5f, .kb_speed = 100.0f, .kb_time = 0.10f, .stun_time = 0.20f },
		.status = { { STATUS_SLOW, 2.5f, 0.45f } },
		.color = { 0.35f, 0.75f, 1.00f },
		.trail = true,
	},

	/* 2 SPARK */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_LINEAR, .speed = 1100.0f },
		.life = 0.35f,
		.hitbox = {.radius = 12.0f },
		.target = {.chain_jumps = 1, .chain_range = 200.0f, .falloff = 0.5f, .min_damage_mul = 0.1f },
		.impact = {.damage_mul = 0.5f, .kb_speed = 150.0f, .kb_time = 0.10f, .stun_time = 0.15f },
		.color = { 1.00f, 0.90f, 0.30f },
		.trail = true,
	},

	// =========================================================== 2 SLOTS
	/* 3 FLAME_NOVA */
	{
		.element = ELEMENT_FIRE,
		.life = 0.35f,
		.hitbox = {.radius = 170.0f, .grow = true },
		.impact = {.damage_mul = 1.5f, .kb_speed = 450.0f, .kb_time = 0.25f, .stun_time = 0.25f },
		.color = { 1.00f, 0.30f, 0.05f },
		.draw_style = DRAW_RING,
	},

	/* 4 FROST_LANCE */
	{
		.element = ELEMENT_ICE,
		.motion = {.mode = MOTION_LINEAR, .speed = 700.0f },
		.life = 1.20f,
		.hitbox = {.radius = 20.0f },
		.target = {.max_pierce = 8 },
		.impact = {.damage_mul = 1.0f, .kb_speed = 140.0f, .kb_time = 0.12f, .stun_time = 0.20f },
		.status = { { STATUS_SLOW, 3.0f, 0.35f } },
		.color = { 0.55f, 0.85f, 1.00f },
		.trail = true,
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
	},

	/* 7 OVERLOAD  */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_ORBIT_SEEK, .speed = 720.0f, .orbit_radius = 80.0f,
					 .spin_speed = 2.8f, .count = 1, .seek_range = 300.0f },
		.life = 10.00f,
		.hitbox = {.radius = 20.0f },
		.target = {.hit_interval = 1.50f },
		.impact = {.damage_mul = 1.5f, .kb_speed = 220.0f, .kb_time = 0.12f, .stun_time = 0.20f },
		.status = { { STATUS_BURN, 2.0f, 0.5f } },
		.color = { 1.00f, 0.75f, 0.35f },
		.trail = true,
	},

	/* 8 STATIC_FROST */
	{
		.element = ELEMENT_ICE,
		.motion = {.mode = MOTION_LINEAR, .speed = 900.0f },
		.life = 0.55f,
		.hitbox = {.radius = 15.0f },
		.target = {.chain_jumps = 2, .chain_range = 210.0f, .falloff = 0.5f, .min_damage_mul = 0.1f },
		.impact = {.damage_mul = 1.0f, .kb_speed = 120.0f, .kb_time = 0.10f, .stun_time = 0.15f },
		.status = { { STATUS_SLOW, 2.5f, 0.5f } },
		.color = { 0.65f, 0.90f, 0.95f },
		.trail = true,
	},

	// =========================================================== 3 SLOTS
	/* 9 METEOR  BURST */
	{
		.element = ELEMENT_FIRE,
		.motion = {.mode = MOTION_AT_CURSOR },
		.life = 0.80f,
		.spawn_delay = 999.0f,
		.hitbox = {.radius = 150.0f },
		.impact = {.damage_mul = 0.0f },
		.child = { SKILL_BIG_METEOR_IMPACT, SPAWN_ON_EXPIRE },
		.color = { 1.00f, 0.35f, 0.05f },
	},

	/* 10 ABSOLUTE_ZERO */
	{
		.element = ELEMENT_ICE,
		.life = 0.50f,
		.hitbox = {.grow = true, .screen_wide = true },
		.impact = {.damage_mul = 1.0f },
		.status = { { STATUS_FREEZE, 1.0f, 0.0f } },
		.color = { 0.70f, 0.95f, 1.00f },
		.hitstop = 0.08f,
		.draw_style = DRAW_RING,
	},

	/* 11 TESLA_TURRET */
	{
		.element = ELEMENT_THUNDER,
		.life = 6.00f,
		.hitbox = {.radius = 200.0f },
		.target = {.hit_interval = 0.50f, .chain_jumps = 2, .chain_range = 170.0f, .falloff = 0.3f, .min_damage_mul = 0.1f },
		.impact = {.damage_mul = 0.75f, .kb_speed = 110.0f, .kb_time = 0.08f, .stun_time = 0.10f },
		.color = { 0.85f, 0.85f, 1.00f },
		.draw_style = DRAW_TESLA,
		.max_instances = 2,
	},

	/* 12 MAGMA_FIELD */
	{
		.element = ELEMENT_FIRE,
		.life = 0.40f,
		.hitbox = {.radius = 80.0f, .grow = true },
		.impact = {.damage_mul = 0.5f, .kb_speed = 380.0f, .kb_time = 0.22f, .stun_time = 0.25f },
		.status = { { STATUS_BURN, 0.5f, 1.0f } },
		.child = { SKILL_MAGMA_GROUND, SPAWN_ON_EXPIRE },
		.color = { 1.00f, 0.35f, 0.05f },
		.hitstop = 0.05f,
		.draw_style = DRAW_RING,
	},

	/* 13 FROSTFIRE_SPIKES */
	{
		.element = ELEMENT_ICE,
		.motion = {.count = 3, .spread = 0.5236f },   // 30 deg apart
		.life = 1.00f,
		.spawn_delay = 0.12f,                            
		.hitbox = {.length = 300.0f, .thickness = 26.0f },
		.impact = {.damage_mul = 1.0f, .kb_speed = 260.0f, .kb_time = 0.18f, .stun_time = 0.25f },
		.status = { { STATUS_SLOW, 2.5f, 0.5f } }, 
		.color = { 0.16f, 0.28f, 0.85f },         
		.hitstop = 0.10f,
		.draw_style = DRAW_DOTS,
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
	},

	/* 15 PLASMA_ORBS */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_ORBIT, .orbit_radius = 90.0f, .spin_speed = 2.5f, .count = 3 },
		.life = 5.00f,
		.hitbox = {.radius = 16.0f },
		.target = {.hit_interval = 0.15f },
		.impact = {.damage_mul = 0.5f, .kb_speed = 120.0f, .kb_time = 0.08f, .stun_time = 0.06f },
		.status = { { STATUS_BURN, 1.5f, 1.0f } },
		.color = { 1.00f, 0.65f, 0.35f },
		.trail = true,
		.max_instances = 1,
	},

	/* 16 GLACIAL_ORBS */
	{
		.element = ELEMENT_ICE,
		.motion = {.mode = MOTION_ORBIT_SEEK, .speed = 620.0f, .orbit_radius = 85.0f, .spin_speed = 2.2f, .count = 2, .seek_range = 280.0f },
		.life = 7.00f,
		.hitbox = {.radius = 22.0f },
		.target = {.hit_interval = 2.20f },
		.impact = {.damage_mul = 0.5f, .kb_speed = 180.0f, .kb_time = 0.12f, .stun_time = 0.20f },
		.status = { { STATUS_SLOW, 2.0f, 0.5f } },
		.child = { SKILL_FROST_BURST, SPAWN_ON_HIT },
		.color = { 0.60f, 0.90f, 1.00f },
		.trail = true,
		.max_instances = 1,
	},

	/* 17 STORMFREEZE */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_FOLLOW_CURSOR, .speed = 900.0f, .count = 1 },
		.life = 8.00f,
		.hitbox = {.radius = 30.0f },
		.target = {.hit_interval = 0.20f },
		.impact = {.damage_mul = 1.0f, .kb_speed = 90.0f, .kb_time = 0.08f, .stun_time = 0.10f },
		.status = { { STATUS_SLOW, 1.2f, 0.55f } },
		.color = { 0.70f, 0.85f, 1.00f },
		.trail = true,
		.max_instances = 1,
	},

	/* 18 PRISM */
	{
		.element = ELEMENT_FIRE,
		.life = 10.00f,
		.hitbox = {.radius = 42.0f }, 
		.impact = {.damage_mul = 0.0f },
		.color = { 0.85f, 0.92f, 1.00f },
		.draw_style = DRAW_RING,
		.shield_time = 10.0f,
		.shield_charges = 1,
	},

	// =========================================================== ULTIMATES
	/* 19 ULT_FIRE_2  meteor shower */
	{
		.element = ELEMENT_FIRE,
		.motion = {.mode = MOTION_AT_CURSOR, .count = 4, .scatter = 150.0f, .stagger = 0.18f },
		.life = 0.70f,
		.spawn_delay = 999.0f,
		.hitbox = {.radius = 80.0f },
		.impact = {.damage_mul = 0.0f },
		.child = { SKILL_METEOR_IMPACT, SPAWN_ON_EXPIRE },
		.color = { 1.00f, 0.40f, 0.10f },
	},

	/* 20 ULT_FIRE_3  meteor storm */
	{
		.element = ELEMENT_FIRE,
		.motion = {.count = 10, .scatter = 420.0f, .stagger = 0.13f },
		.life = 0.70f,
		.spawn_delay = 999.0f,
		.hitbox = {.radius = 80.0f },
		.impact = {.damage_mul = 0.0f },
		.child = { SKILL_METEOR_IMPACT, SPAWN_ON_EXPIRE },
		.color = { 1.00f, 0.30f, 0.02f },
		.hitstop = 0.08f,
	},

	/* 21 ULT_ICE_2  frost nova */
	{
		.element = ELEMENT_ICE,
		.life = 0.60f,
		.hitbox = {.radius = 350.0f, .grow = true },
		.impact = {.damage_mul = 0.5f, .kb_speed = 260.0f, .kb_time = 0.20f, .stun_time = 0.30f },
		.status = { { STATUS_SLOW, 4.0f, 0.35f } },
		.color = { 0.65f, 0.92f, 1.00f },
		.hitstop = 0.06f,
		.draw_style = DRAW_RING,
	},

	/* 22 ULT_ICE_3  glacial beam */
	{
		.element = ELEMENT_ICE,
		.life = 0.55f,
		.spawn_delay = 0.25f,
		.hitbox = {.length = 900.0f, .thickness = 30.0f },
		.impact = {.damage_mul = 2.0f, .kb_speed = 200.0f, .kb_time = 0.15f, .stun_time = 0.30f },
		.status = { { STATUS_SLOW, 2.0f, 0.22f } },
		.color = { 0.45f, 0.85f, 1.00f },
		.hitstop = 0.06f,
		.draw_style = DRAW_BEAM,
	},

	/* 23 ULT_THUNDER_2 */
	{
		.element = ELEMENT_THUNDER,
		.motion = {.mode = MOTION_ORBIT_SEEK, .speed = 780.0f, .orbit_radius = 95.0f, .spin_speed = 3.2f, .count = 2, .seek_range = 240.0f },
		.life = 5.00f,
		.hitbox = {.radius = 22.0f },
		.target = {.hit_interval = 0.30f, .chain_jumps = 2, .chain_range = 190.0f, .falloff = 0.5f, .min_damage_mul = 0.1f },
		.impact = {.damage_mul = 0.3f, .kb_speed = 200.0f, .kb_time = 0.12f, .stun_time = 0.18f },
		.color = { 0.98f, 0.95f, 0.50f },
		.trail = true,
		.max_instances = 1,
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
	},

	// ==================================================== internal children
	/* 25 METEOR_IMPACT */
	{
		.element = ELEMENT_FIRE,
		.life = 0.45f,
		.hitbox = {.radius = 80.0f, .grow = true },
		.impact = {.damage_mul = 2.0f, .kb_speed = 650.0f, .kb_time = 0.30f, .stun_time = 0.35f },
		.status = { { STATUS_BURN, 2.0f, 2.0f } },
		.color = { 1.00f, 0.20f, 0.00f },
		.hitstop = 0.05f,
		.draw_style = DRAW_RING,
	},

	/* 26  BIG METEOR_IMPACT */
	{
		.element = ELEMENT_FIRE,
		.life = 0.45f,
		.hitbox = {.radius = 150.0f, .grow = true },
		.impact = {.damage_mul = 2.0f, .kb_speed = 650.0f, .kb_time = 0.30f, .stun_time = 0.35f },
		.status = { { STATUS_BURN, 2.0f, 2.0f } },
		.color = { 1.00f, 0.20f, 0.00f },
		.hitstop = 0.07f,
		.draw_style = DRAW_RING,
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
		.status = { { STATUS_SLOW, 1.0f, 0.3f }, { STATUS_BURN, 1.0f, 0.5f } },
		.color = { 0.80f, 0.80f, 0.90f },
		.draw_style = DRAW_RING,
	},

	/* 29 MAGMA_GROUND */
	{
		.element = ELEMENT_FIRE,
		.life = 3.00f,
		.hitbox = {.radius = 80.0f },
		.target = {.hit_interval = 0.70f },
		.impact = {.damage_mul = 0.1f },
		.status = { { STATUS_BURN, 1.0f, 0.5f }, { STATUS_SLOW, 0.5f, 0.7f } },
		.color = { 0.90f, 0.25f, 0.05f },
		.draw_style = DRAW_ORB,
		.max_instances = 2,
	},

	/* 30 WHIRLPOOL */
	{
		.element = ELEMENT_FIRE,
		.life = 3.00f,
		.hitbox = {.radius = 150.0f },
		.target = {.hit_interval = 0.30f },
		.impact = {.damage_mul = 0.4f, .pull_speed = 320.0f },
		.status = { { STATUS_BURN, 1.0f, 1.0f } },
		.color = { 1.00f, 0.45f, 0.10f },
		.draw_style = DRAW_RING,
		.max_instances = 2,
	},
};

static constexpr int SKILL_INSTANCE_MAX = 96;
static constexpr int SKILL_PIERCE_MAX = 24;   
static constexpr int SKILL_TRAIL_MAX = 26; 

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
	int      trail_count;

	bool     active;
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
	for (ArcVisual& a : g_Arcs)
	{
		if (a.active) { continue; }
		a.from = from; a.to = to; a.life = ARC_LIFE; a.color = color; a.active = true;
		return;
	}
}

static Vector2 CursorWorld()
{
	return {
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

static void RecordTrail(SkillInstance& s, const SkillDef& def)
{
	if (!def.trail) { return; }

	if (s.trail_count < SKILL_TRAIL_MAX)
	{
		s.trail[s.trail_count++] = s.pos;
		return;
	}

	for (int i = 0; i < SKILL_TRAIL_MAX - 1; i++)
	{
		s.trail[i] = s.trail[i + 1];
	}
	s.trail[SKILL_TRAIL_MAX - 1] = s.pos;
}

static SkillDef ModifiedDef(SkillId id)
{
	SkillDef def = g_SkillDefs[id];
	const ElementMods& m = GameProgress_GetElementMods(def.element);

	def.life *= m.life_mul;
	def.motion.count += m.bonus_count;
	def.impact.damage_mul *= m.damage_mul;    

	if (def.target.max_pierce > 0)
	{
		def.target.max_pierce += m.bonus_pierce;
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

static bool DeliverHit(const SkillInstance& s, const SkillDef& def, int enemy_index, int damage, const Vector2& dir)
{
	HitInfo hit;
	hit.damage = damage;
	hit.hitstun_time = def.impact.stun_time;

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

	const bool killed = GameEnemy_ApplyHit(enemy_index, hit);

	if (!killed)
	{
		for (const StatusApply& fx : def.status)
		{
			if (fx.type == STATUS_NONE) { continue; }
			GameEnemy_ApplyStatus(enemy_index, fx.type, fx.time, fx.mag);
		}
	}
	else
	{
		const Vector2 p = GameEnemy_GetPos(enemy_index);
		GameImpact_Trigger(ExplosionType_Small, p.x, p.y);
		// LATER: XP gem spawn
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

void GameSkill_Cast(SkillId id, const Vector2& origin, const Vector2& aim, int power)
{
	if (id == SKILL_NONE) { return; }

	const SkillDef def = ModifiedDef(id);         
	const int count = std::max(1, def.motion.count);

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

		s.life = def.life;
		s.max_life = def.life;
		s.delay_left = def.spawn_delay + def.motion.stagger * spawned;
		s.visual_radius = def.hitbox.grow ? 0.0f : def.hitbox.radius;
		s.hit_timer = 0.0f;
		s.power = power;
		s.target_id = 0;
		s.seek_cooldown = 0.0f;
		s.hit_count = 0;
		s.trail_count = 0;
		s.active = true;

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

	GameUI_Flash(g_SkillDefs[id].color, 0.28f);

	GameSkill_Cast(id, origin, aim, GameProgress_GetElementLevel(e) * 3);
}

// ============================================================================
// Damage
// ============================================================================
static void DamagePass(SkillInstance& s, const SkillDef& def)
{
	int targets_hit = 0;

	for (int i = 0; i < GameEnemy_GetActiveCount(); i++)
	{
		const int enemy_id = GameEnemy_GetId(i);
		if (AlreadyHit(s, enemy_id)) { continue; }

		const CollisionCircle cc = GameEnemy_GetCollisionCircle(i);
		const Vector2 enemy_pos = { cc.position.x, cc.position.y };
		if (!InHitbox(s, def, enemy_pos, cc.radius)) { continue; }

		float mul = 1.0f;
		for (int k = 0; k < targets_hit; k++) { mul *= def.target.falloff; }
		mul = std::max(mul, def.target.min_damage_mul);

		const int damage = std::max(1,
			static_cast<int>(s.power * def.impact.damage_mul * mul));
		const Vector2 offset = enemy_pos - s.pos;
		const Vector2 dir = (offset.LengthSq() < 0.01f) ? s.facing : Vector2_Normalize(offset);

		DeliverHit(s, def, i, damage, dir);
		RememberHit(s, enemy_id);
		targets_hit++;

		// child spawned at the target
		if (def.child.id != SKILL_NONE && (def.child.trigger == SPAWN_ON_HIT || def.child.trigger == SPAWN_ON_TICK))
		{
			GameSkill_Cast(def.child.id, enemy_pos, dir, s.power);
		}

		if (def.target.chain_jumps > 0) { SpawnArc(s.pos, enemy_pos, def.color); }

		// chain: each hop searches from the LAST enemy hit, walking outward
		Vector2 arc_from = enemy_pos;
		for (int jump = 0; jump < def.target.chain_jumps; jump++)
		{
			int   best = -1;
			float best_sq = def.target.chain_range * def.target.chain_range;

			for (int j = 0; j < GameEnemy_GetActiveCount(); j++)
			{
				if (AlreadyHit(s, GameEnemy_GetId(j))) { continue; }

				const float d_sq = (GameEnemy_GetPos(j) - arc_from).LengthSq();
				if (d_sq < best_sq) { best_sq = d_sq; best = j; }
			}
			if (best < 0) { break; }   // nothing left in range: chain ends

			float arc_mul = 1.0f;
			for (int k = 0; k < targets_hit; k++) { arc_mul *= def.target.falloff; }
			arc_mul = std::max(arc_mul, def.target.min_damage_mul);

			const Vector2 arc_pos = GameEnemy_GetPos(best);
			const Vector2 arc_dir = Vector2_Normalize(arc_pos - arc_from);

			DeliverHit(s, def, best,
				std::max(1, static_cast<int>(s.power * def.impact.damage_mul * arc_mul)),
				arc_dir);
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
	for (ArcVisual& a : g_Arcs)
	{
		if (!a.active) { continue; }
		a.life -= delta_time;
		if (a.life <= 0.0f) { a.active = false; }
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
				GameSkill_Cast(def.child.id, s.pos, s.facing, s.power);
			}
			continue;
		}

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
				s.orbit_angle += def.motion.spin_speed * delta_time;
				s.pos = PlayerPos() + Vector2_FromAngle(s.orbit_angle) * def.motion.orbit_radius;

				if (s.seek_cooldown <= 0.0f && s.hit_timer <= 0.0f)
				{
					const float seek_sq = def.motion.seek_range * def.motion.seek_range;
					for (int i = 0; i < GameEnemy_GetActiveCount(); i++)
					{
						if ((GameEnemy_GetPos(i) - s.pos).LengthSq() < seek_sq)
						{
							s.target_id = GameEnemy_GetId(i);
							break;
						}
					}
				}
			}
			else
			{
				const int idx = GameEnemy_FindById(s.target_id);
				if (idx < 0)
				{
					s.target_id = 0;     
					break;
				}

				const Vector2 to_target = GameEnemy_GetPos(idx) - s.pos;
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
					s.target_id = 0;
					s.hit_timer = def.target.hit_interval;
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
void GameSkill_Draw()
{
	for (const ArcVisual& a : g_Arcs)
	{
		if (!a.active) { continue; }

		const float fade = a.life / ARC_LIFE;
		DrawPrim_Beam(a.from, a.to, 2.0f + 3.0f * fade, a.color, fade);
	}

	for (const SkillInstance& s : g_Instances)
	{
		if (!s.active) { continue; }

		const SkillDef& def = s.def;

		// --- screen-wide ring out from the player ---
		if (def.hitbox.screen_wide)
		{
			const float r = 400.0f * (1.0f - (s.life / s.max_life));
			DrawPrim_Ring(PlayerPos(), r, 7.0f, def.color, 0.85f);
			DrawPrim_Ring(PlayerPos(), r * 0.82f, 3.0f, def.color, 0.45f);
			continue;
		}

		// --- telegraph: where it WILL land, filled disc
		if (s.delay_left > 0.0f && def.spawn_delay > 0.0f)
		{
			if (def.hitbox.length > 0.0f) { continue; }

			const float t = std::min(s.delay_left / def.spawn_delay, 1.0f);
			const float r = (def.hitbox.radius > 0.0f) ? def.hitbox.radius : def.hitbox.length * 0.5f;

			DrawPrim_Ring(s.pos, r, 3.0f, def.color, 0.9f);
			DrawPrim_Circle(s.pos, r * (1.0f - t), def.color, 0.30f);
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
					DrawPrim_Circle(p, r * 0.42f, { 0.62f, 0.82f, 1.0f }, 0.9f);   // icy core
					DrawPrim_Ring(p, r * 1.25f, 2.0f, def.color, 0.4f);
				}
			}
			else
			{
				DrawPrim_Beam(a, b, def.hitbox.thickness, def.color, 1.0f);
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
			DrawPrim_Trail(s.trail, s.trail_count,
				def.hitbox.radius * 0.8f, def.color, 0.8f, 0.0f);
		}

		// --- circle ---
		if (def.draw_style == DRAW_RING)
		{
			DrawPrim_Ring(s.pos, s.visual_radius, 3.0f, def.color, 0.85f);
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