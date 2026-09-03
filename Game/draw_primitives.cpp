/*============================================================================
Contents   :  [draw_primitives.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/15
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>

#include "draw_primitives.h"
#include "texture.h"
#include "sprite.h"
#include "camera.h"
#include "debug_ostream.h"

using namespace DirectX;

static int g_white_texture_id = -1;

static constexpr float TWO_PI_PRIM = 6.2831853f;

void DrawPrim_Initialize()
{
	g_white_texture_id = Texture_Load(L"assets/textures/white2.png", false);
}

void DrawPrim_Finalize()
{
	Texture_Release(g_white_texture_id);
}

void DrawPrim_Line(const Vector2& from, const Vector2& to, float thickness,
	const XMFLOAT3& color, float alpha, float overlap)
{
	const Vector2 delta = to - from;
	const float   len = delta.Length();
	if (len < 0.5f) { return; }
	if (len > 3000.0f) { return; }

	DrawPrim_Rect(from + delta * 0.5f,
		len * overlap, thickness * 2.0f,
		Vector2_ToAngle(delta),
		color, alpha);
}

void DrawPrim_Rect(const Vector2& center, float width, float height, float angle,
	const XMFLOAT3& color, float alpha)
{
	if (width <= 0.0f || height <= 0.0f || alpha <= 0.0f) { return; }
	if (g_white_texture_id == TEXTURE_INVALID_ID) { return; }

	SpriteDrawParams p;
	p.color = color;
	p.alpha = alpha;
	p.angle = angle;

	Sprite_Draw(
		g_white_texture_id,
		Camera_WorldToScreenX(center.x - width * 0.5f),
		Camera_WorldToScreenY(center.y - height * 0.5f),
		width, height,
		p);
}

void DrawPrim_Beam(const Vector2& from, const Vector2& to, float thickness,
	const XMFLOAT3& color, float alpha)
{
	DrawPrim_Line(from, to, thickness * 1.7f, color, alpha * 0.35f);   // glow
	DrawPrim_Line(from, to, thickness, color, alpha);          // core

	// a brighter, thinner heart makes it look hot rather than painted
	const XMFLOAT3 hot{
		std::min(1.0f, color.x + 0.45f),
		std::min(1.0f, color.y + 0.45f),
		std::min(1.0f, color.z + 0.45f)
	};
	DrawPrim_Line(from, to, thickness * 0.35f, hot, alpha);
}

void DrawPrim_Circle(const Vector2& center, float radius,
	const XMFLOAT3& color, float alpha)
{
	if (radius <= 0.5f || alpha <= 0.0f) { return; }

	constexpr int SLICES = 12;
	const float slice_h = (radius * 2.0f) / SLICES;

	for (int i = 0; i < SLICES; i++)
	{
		// y from -radius to +radius through the middle of this slice
		const float y = -radius + slice_h * (i + 0.5f);

		// half-chord at height y: sqrt(r^2 - y^2)
		const float half_chord = sqrtf(std::max(0.0f, radius * radius - y * y));
		if (half_chord <= 0.5f) { continue; }

		DrawPrim_Rect({ center.x, center.y + y },
			half_chord * 2.0f, slice_h + 1.0f,  
			0.0f, color, alpha);
	}
}

void DrawPrim_Trail(const Vector2* points, int count, float thickness,
	const XMFLOAT3& color, float alpha_head, float alpha_tail)
{
	if (points == nullptr || count < 2) { return; }

	for (int i = 0; i < count - 1; i++)
	{
		const float t = static_cast<float>(i + 1) / static_cast<float>(count - 1);
		const float a = alpha_tail + (alpha_head - alpha_tail) * t;
		const float w = thickness * (0.25f + 0.75f * t);

		DrawPrim_Line(points[i], points[i + 1], w, color, a, 1.18f);
	}
}

void DrawPrim_Ring(const Vector2& center, float radius, float thickness,
	const XMFLOAT3& color, float alpha)
{
	if (radius <= 0.5f || alpha <= 0.0f) { return; }

	// Segments scale with size
	const int segments = std::clamp(static_cast<int>(radius * 0.35f), 10, 36);
	const float step = TWO_PI_PRIM / segments;

	const float seg_len = radius * step * 1.15f;

	for (int i = 0; i < segments; i++)
	{
		const float a = step * i;
		const Vector2 p{ center.x + cosf(a) * radius, center.y + sinf(a) * radius };
		DrawPrim_Rect(p, seg_len, thickness * 2.0f, a + 1.5708f, color, alpha);
	}
}
