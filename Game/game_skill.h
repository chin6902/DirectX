/*============================================================================
Contents   :  [game_skill.h]

Author     : Chin Qing You
LastUpdate : 2026/08/16
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_SKILL_H
#define GAME_SKILL_H

#include "element.h"
#include "vector2.h"

enum SkillId
{
	SKILL_NONE = -1,

	// --- 1 slot ---
	SKILL_FIRE_BOLT,        // {1,0,0} fast, short range, one target
	SKILL_ICE_SHARD,        // {0,1,0} slow projectile, slows one
	SKILL_SPARK,            // {0,0,1} hits one, chain once

	// --- 2 slots ---
	SKILL_FLAME_NOVA,       // {2,0,0} burst around the player
	SKILL_FROST_LANCE,      // {0,2,0} pierces , slows all hit
	SKILL_CHAIN_BOLT,       // {0,0,2} chain more 
	SKILL_STEAM_BURST,      // {1,1,0} hit -> explosion -> steam cloud
	SKILL_OVERLOAD,         // {1,0,1} summon that burn for 10s
	SKILL_STATIC_FROST,     // {0,1,1} chain that slows every link

	// --- 3 slots ---
	SKILL_METEOR,           // {3,0,0} meteor strike at the cursor
	SKILL_ABSOLUTE_ZERO,    // {0,3,0} screen-wide freeze
	SKILL_TESLA_TURRET,     // {0,0,3} placed turret, chain nearby enemies
	SKILL_MAGMA_FIELD,      // {2,1,0} burning ground
	SKILL_FROSTFIRE_SPIKES, // {1,2,0} 3 way spike that slows
	SKILL_FIRESTORM,        // {2,0,1} hit -> explosion -> whirlpool
	SKILL_PLASMA_ORBS,      // {1,0,2} 3 orbit orbs with burn
	SKILL_GLACIAL_ORBS,     // {0,2,1} 2 ice orbs that dive and shatter
	SKILL_STORMFREEZE,      // {0,1,2} orb with the cursor control
	SKILL_PRISM,            // {1,1,1} shield

	// --- ultimates
	SKILL_ULT_FIRE_2,       // meteor shower: 4 meteor
	SKILL_ULT_FIRE_3,       // meteor storm: 10 meteor
	SKILL_ULT_ICE_2,        // area slow
	SKILL_ULT_ICE_3,        // beam
	SKILL_ULT_THUNDER_2,    // tracking orbs with chain damage
	SKILL_ULT_THUNDER_3,    // bouncing orb

	// --- internal
	SKILL_METEOR_IMPACT,
	SKILL_BIG_METEOR_IMPACT,
	SKILL_FROST_BURST,
	SKILL_STEAM_CLOUD,
	SKILL_MAGMA_GROUND,
	SKILL_WHIRLPOOL,

	SKILL_ID_COUNT,
};

void GameSkill_Initialize();
void GameSkill_Finalize();
void GameSkill_Update(float delta_time);
void GameSkill_Draw();
void GameSkill_DrawUnder();

SkillId GameSkill_Lookup(const int* counts);
void GameSkill_Cast(SkillId id, const Vector2& origin, const Vector2& aim, int power, unsigned int element_mask = 0);

void GameSkill_CastUltimate(ElementType e, const Vector2& origin, const Vector2& aim);

#endif