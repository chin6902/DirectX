/*============================================================================
Contents   :  [game_time.h]
              
Author     : Chin Qing You
LastUpdate : 2026/07/31
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_TIME_H
#define GAME_TIME_H

float GameTime_Scale(float raw_delta_time);

void GameTime_SetScale(float scale);
float GameTime_GetScale();

void GameTime_Hitstop(float duration);

void GameTime_Initialize();

#endif