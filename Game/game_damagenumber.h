/*============================================================================
Contents   :  [game_damagenumber.h]

Author     : Chin Qing You
LastUpdate : 2026/08/21
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_DAMAGENUMBER_H
#define GAME_DAMAGENUMBER_H

#include <DirectXMath.h>

#include "vector2.h"

void GameDamageNumber_Initialize();
void GameDamageNumber_Finalize();
void GameDamageNumber_Update(float delta_time);
void GameDamageNumber_Draw();

void GameDamageNumber_Spawn(const Vector2& world_pos, float damage, int target_id, const DirectX::XMFLOAT3& color);

int GameDamageNumber_GetActiveCount();

#endif