/*============================================================================
Contents   :  [game_enemy.h]

Author     : Chin Qing You
LastUpdate : 2026/07/01
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_ENEMY_H
#define GAME_ENEMY_H

void GameEnemy_Initialize();
void GameEnemy_Finalize();

enum EnemyType
{
	EnemyType_Normal,
	EnemyType_Fast,
};

void GameEnemy_Update(float delta_time);
void GameEnemy_Draw();

void GameEnemy_Create(EnemyType type, float startX, float startY);

#endif
