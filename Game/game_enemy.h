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
#include "vector2.h"

enum EnemyType
{
	EnemyType_Invalid = -1,
	EnemyType_CHASER,
	EnemyType_ORBITER,
	ENEMY_TYPE_COUNT
};

struct HitInfo
{
	int damage = 1;
	float knockback_speed = 0.0f;
	float knockback_time = 0.0f;
	float hitstun_time = 0.0f;
	Vector2 direction = { 1.0f, 0.0f };
};

enum StatusType
{
	STATUS_NONE = -1,
	STATUS_BURN,      
	STATUS_SLOW,      
	STATUS_FREEZE,    
	STATUS_TYPE_COUNT,
};

void GameEnemy_Initialize();
void GameEnemy_Finalize();
void GameEnemy_Update(float delta_time);
void GameEnemy_Draw();

void GameEnemy_Create(EnemyType type, const Vector2& pos);

int GameEnemy_GetActiveCount();
int GameEnemy_GetId(int index);
int GameEnemy_FindById(int id);
int GameEnemy_GetContactDamage(int index);
Vector2 GameEnemy_GetPos(int index);
int GameEnemy_GetContactDamage(int index);

bool GameEnemy_ApplyHit(int index, const HitInfo& hit_info);
void GameEnemy_ApplyStatus(int index, StatusType type, float duration, float magnitude);

void GameEnemy_Destroy(int enemyIndex);
void GameEnemy_CleanUp();

CollisionCircle GameEnemy_GetCollisionCircle(int enemyIndex);
ExplosionType GameEnemy_GetExplosionType(int enemyIndex);

#endif
