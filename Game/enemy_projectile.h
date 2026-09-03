/*============================================================================
Contents   :  [enemy_projectile.h]

Author     : Chin Qing You
LastUpdate : 2026/08/18
-----------------------------------------------------------------------------

============================================================================*/
#ifndef ENEMY_PROJECTILE_H
#define ENEMY_PROJECTILE_H

#include <DirectXMath.h>

#include "vector2.h"

void EnemyProjectile_Initialize();
void EnemyProjectile_Finalize();
void EnemyProjectile_Update(float delta_time);
void EnemyProjectile_Draw();

void EnemyProjectile_Fire(const Vector2& pos, const Vector2& dir,
    float speed, int damage,
    const DirectX::XMFLOAT3& color);

int EnemyProjectile_GetActiveCount();

#endif