/*============================================================================
Contents   :  [collision.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/04/15
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>

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

bool Collision_IsOverlap(const CollisionRect& a, const CollisionRect& b)
{
	if (a.max.x <= b.min.x) return false;
	if (a.min.x >= b.max.x) return false;
	if (a.max.y <= b.min.y) return false;
	if (a.min.y >= b.max.y) return false;

	return true;
}

// For collision with stage
HitData Collision_IsHit(const CollisionRect& a, const CollisionRect& b)
{
	HitData hit{};
	hit.isHit = false;
	hit.normal = { 0.0f, 0.0f };
	hit.rectangle = b;

	if (!Collision_IsOverlap(a, b))
	{
		return hit;
	}

	hit.isHit = true;

	float overlapLeft = a.max.x - b.min.x; // overlap from the left side of a to the right side of b
	float overlapRight = b.max.x - a.min.x; // overlap from the right side of a to the left side of b
	float overlapTop = a.max.y - b.min.y; // overlap from the top side of a to the bottom side of b
	float overlapBottom = b.max.y - a.min.y; // overlap from the bottom side of a to the top side of b

	float overlapX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
	float overlapY = (overlapTop < overlapBottom) ? overlapTop : overlapBottom;

	// Determine the collision normal based on the smallest overlap
	if(overlapX < overlapY)
	{
		// Collision is more horizontal than vertical
		if (overlapLeft < overlapRight)
		{
			hit.normal = { -1.0f, 0.0f }; // Hit from left
		}
		// Collision is more vertical than horizontal
		else
		{
			hit.normal = { 1.0f, 0.0f }; // Hit from right
		}
	}
	else
	{
		// Collision is more vertical than horizontal
		if (overlapTop < overlapBottom)
		{
			hit.normal = { 0.0f, -1.0f }; // Hit from top
		}
		// Collision is more horizontal than vertical
		else
		{
			hit.normal = { 0.0f, 1.0f }; // Hit from bottom
		}
	}

	return hit;
}

float Collision_PointSegmentDistanceSq(const DirectX::XMFLOAT2& point, const DirectX::XMFLOAT2& seg_start, const DirectX::XMFLOAT2& seg_end)
{
	const float sx = seg_end.x - seg_start.x;
	const float sy = seg_end.y - seg_start.y;
	const float len_sq = sx * sx + sy * sy;

	if (len_sq < 0.0001f)
	{
		// Degenerate segment: it is a point.
		const float dx = point.x - seg_start.x;
		const float dy = point.y - seg_start.y;
		return dx * dx + dy * dy;
	}

	// Project the point onto the infinite line, expressed as a fraction t along the segment, then clamp t to [0,1] 
	float t = ((point.x - seg_start.x) * sx + (point.y - seg_start.y) * sy) / len_sq;
	t = std::clamp(t, 0.0f, 1.0f);

	// Closest point on the segment, then plain distance to it.
	const float cx = seg_start.x + sx * t;
	const float cy = seg_start.y + sy * t;
	const float dx = point.x - cx;
	const float dy = point.y - cy;
	return dx * dx + dy * dy;
}

bool Collision_IsOverlap(const CollisionCapsule& capsule, const CollisionCircle& circle)
{
	const float d_sq = Collision_PointSegmentDistanceSq(
		circle.position, capsule.start, capsule.end);

	const float reach = capsule.half_thickness + circle.radius;
	return d_sq < reach * reach;
}
