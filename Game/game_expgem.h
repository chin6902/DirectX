/*============================================================================
Contents   :  [game_expgem.h]

Author     : Chin Qing You
LastUpdate : 2026/08/17
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_EXPGEM_H
#define GAME_EXPGEM_H

#include "vector2.h"

void GameExpGem_Initialize();
void GameExpGem_Finalize();
void GameExpGem_Update(float delta_time);
void GameExpGem_Draw();

void GameExpGem_Spawn(const Vector2& pos, int xp_value);

int  GameExpGem_GetActiveCount();  

#endif