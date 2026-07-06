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

void Game_Impact_Create();
void Game_Impact_Initialize();
void Game_Impact_Finalize();
void Game_Impact_Trigger(ExplosionType type, float x, float y);
void Game_Impact_Update(float delta_time);
void Game_Impact_Draw();

#endif


