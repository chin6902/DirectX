/*============================================================================
Contents   :  [collision_debug.h]
              
Author     : Chin Qing You
LastUpdate : 2026/07/06
-----------------------------------------------------------------------------

============================================================================*/
#ifndef COLLISION_DEBUG_H
#define COLLISION_DEBUG_H

#include "collision.h"

void Collision_Debug_Initialize();
void Collision_Debug_Finalize();

void Collision_Debug_Draw(const CollisionCircle& circle, const DirectX::XMFLOAT3& color);

void Collision_Debug_Draw(const CollisionCapsule& capsule, const DirectX::XMFLOAT3& color);


#endif
