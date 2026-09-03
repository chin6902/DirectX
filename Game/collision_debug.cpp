/*============================================================================
Contents   :  [collision_debug.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/20
-----------------------------------------------------------------------------

============================================================================*/
#include <DirectXMath.h>
#include <cmath>

#include "collision_debug.h"
#include "config.h"
#include "texture.h"
#include "sprite.h"

using namespace DirectX;

static int g_WhiteTextureID = TEXTURE_INVALID_ID;

static constexpr int   RING_SEGMENTS = 16;
static constexpr float RING_THICKNESS = 2.0f;
static constexpr float SEGMENT_OVERLAP = 1.18f;   

void Collision_Debug_Initialize()
{
	g_WhiteTextureID = Texture_Load(L"assets/textures/white_debug.png", false);
}

void Collision_Debug_Finalize()
{
	Texture_Release(g_WhiteTextureID);
	g_WhiteTextureID = TEXTURE_INVALID_ID;
}

static void DrawBar(float x0, float y0, float x1, float y1,
	float thickness, const XMFLOAT3& color, float alpha)
{
	const float dx = x1 - x0;
	const float dy = y1 - y0;
	const float len = sqrtf(dx * dx + dy * dy);
	if (len < 0.5f) { return; }

	SpriteDrawParams p;
	p.color = color;
	p.alpha = alpha;
	p.angle = atan2f(dy, dx);

	Sprite_Draw(
		g_WhiteTextureID,
		(x0 + x1) * 0.5f - len * 0.5f,
		(y0 + y1) * 0.5f - thickness * 0.5f,
		len, thickness,
		p);
}

void Collision_Debug_Draw(const CollisionCircle& circle, const XMFLOAT3& color)
{
	if (g_WhiteTextureID == TEXTURE_INVALID_ID) { return; }
	if (circle.radius < 0.5f) { return; }

	const float step = XM_2PI / RING_SEGMENTS;

	for (int i = 0; i < RING_SEGMENTS; i++)
	{
		const float a0 = step * i;
		const float a1 = step * (i + 1);

		const float x0 = circle.position.x + cosf(a0) * circle.radius;
		const float y0 = circle.position.y + sinf(a0) * circle.radius;
		const float x1 = circle.position.x + cosf(a1) * circle.radius;
		const float y1 = circle.position.y + sinf(a1) * circle.radius;

		const float mx = (x0 + x1) * 0.5f;
		const float my = (y0 + y1) * 0.5f;

		DrawBar(mx + (x0 - mx) * SEGMENT_OVERLAP, my + (y0 - my) * SEGMENT_OVERLAP,
			mx + (x1 - mx) * SEGMENT_OVERLAP, my + (y1 - my) * SEGMENT_OVERLAP,
			RING_THICKNESS, color, 1.0f);
	}
}

void Collision_Debug_Draw(const CollisionCapsule& capsule, const XMFLOAT3& color)
{
	constexpr int STEPS = 8;

	for (int i = 0; i <= STEPS; i++)
	{
		const float t = static_cast<float>(i) / STEPS;

		const CollisionCircle c{
			{ capsule.start.x + (capsule.end.x - capsule.start.x) * t,
			  capsule.start.y + (capsule.end.y - capsule.start.y) * t },
			capsule.half_thickness
		};

		Collision_Debug_Draw(c, color);
	}
}