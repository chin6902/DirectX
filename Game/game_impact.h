/*============================================================================
Contents   :  [game_impact.h]
              
Author     : Chin Qing You
LastUpdate : 2026/07/06
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_IMPACT_H
#define GAME_IMPACT_H

enum ExplosionType
{
	ExplosionType_Large,
	ExplosionType_Small,
};

void GameImpact_Create();
void GameImpact_Initialize();
void GameImpact_Finalize();
void GameImpact_Trigger(ExplosionType type, float x, float y);
void GameImpact_Update(float delta_time);
void GameImpact_Draw();

#endif


