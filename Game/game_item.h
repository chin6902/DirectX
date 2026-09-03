/*============================================================================
Contents   :  [game_item.h]

Author     : Chin Qing You
LastUpdate : 2026/08/24
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_ITEM_H
#define GAME_ITEM_H

#include "vector2.h"
#include "game_progress.h"   

void GameItem_Initialize();
void GameItem_Finalize();
void GameItem_Update(float delta_time);
void GameItem_Draw();                      

void GameItem_OnEnemyKilled(const Vector2& pos);

float GameItem_GetDamageMul();
float GameItem_GetChargeTimeMul();

bool  GameItem_IsBuffActive();
float GameItem_GetBuffRemaining();          
float GameItem_GetBuffDuration();           
int   GameItem_GetBuffIconTexture();        

int   GameItem_GetItemTexture(ItemType item);
#endif