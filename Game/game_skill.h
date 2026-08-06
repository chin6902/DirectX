/*============================================================================
Contents   :  [game_skill.h]
              
Author     : Chin Qing You
LastUpdate : 2026/08/06
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_SKILL_H
#define GAME_SKILL_H

#include "vector2.h"

enum SkillId
{
	SKILL_NONE = -1,
	SKILL_FIRE_BOLT,      // {1,0,0}
	SKILL_FLAME_NOVA,     // {2,0,0}
	SKILL_INFERNO,        // {3,0,0}
	SKILL_ICE_SHARD,      // {0,1,0}
	SKILL_ICE_SPIKE,       // {0,2,0}
	SKILL_BLIZZARD,       // {0,3,0}
	SKILL_THUNDER_BOLT,    // {0,0,1}
	SKILL_LIGHTNING_CHAIN,  // {0,0,2}
	SKILL_THUNDERSTORM,    // {0,0,3}
	SKILL_ID_COUNT,
};

void GameSkill_Initialize();
void GameSkill_Finalize();
void GameSkill_Update(float delta_time);
void GameSkill_Draw();

// counts[ELEMENT_TYPE_COUNT] -> matching recipe, or SKILL_NONE
SkillId GameSkill_Lookup(const int* counts);

void GameSkill_Cast(SkillId id, const Vector2& origin, const Vector2& aim, int power);

#endif