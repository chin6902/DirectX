/*============================================================================
Contents   :  [enemy_formation.h]

Author     : Chin Qing You
LastUpdate : 2026/08/18
-----------------------------------------------------------------------------

============================================================================*/
#ifndef ENEMY_FORMATION_H
#define ENEMY_FORMATION_H

#include "vector2.h"

void EnemyFormation_Initialize();
void EnemyFormation_Finalize();
void EnemyFormation_Update(float delta_time);
void EnemyFormation_Draw();

void EnemyFormation_SpawnTriangle(const Vector2& origin, const Vector2& dir,
    float speed, int rows);

void EnemyFormation_SpawnWall(const Vector2& origin, float dir_x, float speed);

Vector2 EnemyFormation_GetOrigin(int formation_id);
bool    EnemyFormation_IsActive(int formation_id);

int  EnemyFormation_GetActiveCount();
void EnemyFormation_ClearAll();

#endif