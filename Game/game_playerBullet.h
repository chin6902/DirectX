/*============================================================================
Contents   :  [game_playerBullet.h]
              
Author     : Chin Qing You
LastUpdate : 2026/06/29
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_PLAYER_BULLET_H
#define GAME_PLAYER_BULLET_H

#include "collision.h"

void GamePlayerBullet_Initialize();
void GamePlayerBullet_Finalize();

void GamePlayerBullet_Update(float delta_time);
void GamePlayerBullet_Draw();

void GamePlayerBullet_Create(float startX, float startY);

int GamePlayerBullet_GetActiveCount();
void GamePlayerBullet_Destroy(int bulletIndex);
void GamePlayerBullet_CleanUp();

CollisionCircle GamePlayerBullet_GetCollisionCircle(int bulletIndex);

#endif
