/*============================================================================
Contents   :  [game_enemy.h]

Author     : Chin Qing You
LastUpdate : 2026/07/01
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_ENEMY_H
#define GAME_ENEMY_H

#include "collision.h"
#include "game_impact.h"

void GameEnemy_Initialize();
void GameEnemy_Finalize();

enum EnemyType
{
	EnemyType_Invalid = -1,
	EnemyType_Normal,
	EnemyType_Fast,
};

void GameEnemy_Update(float delta_time);
void GameEnemy_Draw();

void GameEnemy_Create(EnemyType type, float startX, float startY);

int GameEnemy_GetActiveCount();
void GameEnemy_Destroy(int enemyIndex);
void GameEnemy_CleanUp();

CollisionCircle GameEnemy_GetCollisionCircle(int enemyIndex);
ExplosionType GameEnemy_GetExplosionType(int enemyIndex);

#endif
