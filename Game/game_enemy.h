/*============================================================================
Contents   :  [game_enemy.h]

Author     : Chin Qing You
LastUpdate : 2026/08/10
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_ENEMY_H
#define GAME_ENEMY_H

#include "collision.h"
#include "vector2.h"
#include "element.h"

enum EnemyType
{
	ENEMY_TYPE_CHASER,    
	ENEMY_TYPE_ORBITER,   
	ENEMY_TYPE_ELITE,     
	ENEMY_TYPE_DUMMY,
	ENEMY_TYPE_COUNT,
};

inline unsigned int ElementBit(ElementType e) { return 1u << static_cast<int>(e); }

enum StatusType
{
	STATUS_NONE = -1,
	STATUS_BURN,     
	STATUS_SLOW,      
	STATUS_FREEZE,   
	STATUS_TYPE_COUNT,
};

struct HitInfo
{
	float damage = 1.0f;

	unsigned int element_mask = 0;

	float knockback_speed = 0.0f;   
	float knockback_time = 0.0f;  
	float hitstun_time = 0.0f;   
	Vector2 direction = { 1.0f, 0.0f };
};

void GameEnemy_Initialize();
void GameEnemy_Finalize();
void GameEnemy_Update(float delta_time);
void GameEnemy_Draw();

void GameEnemy_Create(EnemyType type, const Vector2& pos);

// --- formation ---
void GameEnemy_CreateInFormation(EnemyType type, int formation_id, const Vector2& offset);
int  GameEnemy_CountInFormation(int formation_id);

int GameEnemy_GetActiveCount();
CollisionCircle GameEnemy_GetCollisionCircle(int index);
Vector2 GameEnemy_GetPos(int index);
int GameEnemy_GetContactDamage(int index);

int GameEnemy_GetId(int index);     
int GameEnemy_FindById(int id);     

bool GameEnemy_ApplyHit(int index, const HitInfo& hit);  
void GameEnemy_ApplyStatus(int index, StatusType type, float duration, float magnitude);

// --- elite shield ---
bool GameEnemy_HasShield(int index);
ElementType GameEnemy_GetShieldElement(int index);

int  GameEnemy_QueryRadius(const Vector2& center, float radius, int* out, int out_max); 

void GameEnemy_SetHPScale(float scale);
int GameEnemy_CountOfType(EnemyType type);  
bool GameEnemy_UsesAttack(int index);

void GameEnemy_Destroy(int index);
void GameEnemy_CleanUp();  
void GameEnemy_BuildGrid();

#endif