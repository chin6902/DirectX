/*============================================================================
Contents   :  [collision.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/04/15
-----------------------------------------------------------------------------

============================================================================*/
#include "collision.h"

using namespace DirectX;

bool Collision_IsOverlap(const CollisionCircle& a, const CollisionCircle& b)
{
	XMVECTOR pos_a = XMLoadFloat2(&a.position);
	XMVECTOR pos_b = XMLoadFloat2(&b.position);

	float lengthSq = XMVectorGetX(XMVector2LengthSq(pos_a - pos_b));
	float radiusSum = a.radius + b.radius;
	return lengthSq < radiusSum * radiusSum;
}
