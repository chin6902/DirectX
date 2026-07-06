/*============================================================================
Contents   :  [collision.h]
              
Author     : Chin Qing You
LastUpdate : 2026/07/01
-----------------------------------------------------------------------------

============================================================================*/
#ifndef COLLISION_H
#define COLLISION_H

#include <DirectXMath.h>

struct CollisionCircle
{
	DirectX::XMFLOAT2 position;
	float radius;
};

bool Collision_IsOverlap(const CollisionCircle& a, const CollisionCircle& b);

#endif
