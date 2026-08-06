/*============================================================================
Contents   :  [game_player.h]
              
Author     : Chin Qing You
LastUpdate : 2026/06/24
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

#include "vector2.h"

void GamePlayer_Initialize(float startX, float startY);
void GamePlayer_Finalize();
void GamePlayer_Update(float delta_time);
//void GamePlayer_FixedUpdate();
void GamePlayer_Draw();

float GamePlayer_GetPosX();
float GamePlayer_GetPosY();
float GamePlayer_GetSpeed();
Vector2 GamePlayer_GetAimDir();

void GamePlayer_SetSpeed(float speed);  

#endif
