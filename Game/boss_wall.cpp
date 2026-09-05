/*============================================================================
Contents   :  [boss_wall.cpp]

Author     : Chin Qing You
LastUpdate : 2026/09/04
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "boss_wall.h"
#include "boss_visual.h"
#include "draw_primitives.h"

using namespace DirectX;

static constexpr int   WALL_SEGMENTS = 16;
static constexpr float WALL_RING_RADIUS = 280.0f;
static constexpr float WALL_SEGMENT_RADIUS = 48.0f;

static constexpr float WALL_LIFE = 10.0f;
static constexpr float WALL_RISE_TIME = 0.35f;   
static constexpr float WALL_WARN_TIME = 2.00f;   

struct WallSegment
{
	Vector2     pos;
	ElementType element = ELEMENT_FIRE;
	bool        alive = false;
};

static WallSegment g_Segments[WALL_SEGMENT_MAX]{};
static bool  g_Active = false;
static float g_Life = 0.0f;
static float g_Age = 0.0f;

static bool ValidSegment(int index)
{
	return index >= 0 && index < WALL_SEGMENTS && g_Segments[index].alive;
}

static float RiseScale()
{
	if (g_Age >= WALL_RISE_TIME) { return 1.0f; }
	return g_Age / WALL_RISE_TIME;
}

void BossWall_Initialize()
{
	BossWall_ClearAll();
}

void BossWall_Finalize()
{
	BossWall_ClearAll();
}

void BossWall_ClearAll()
{
	for (WallSegment& s : g_Segments) { s.alive = false; }
	g_Active = false;
	g_Life = 0.0f;
	g_Age = 0.0f;
}

bool BossWall_IsActive() { return g_Active; }

void BossWall_SpawnRing(const Vector2& center)
{
	BossWall_ClearAll();

	const int offset = rand() % ELEMENT_TYPE_COUNT;

	for (int i = 0; i < WALL_SEGMENTS; i++)
	{
		const float angle = (XM_2PI * static_cast<float>(i)) / static_cast<float>(WALL_SEGMENTS);

		WallSegment& s = g_Segments[i];
		s.pos = center + Vector2_FromAngle(angle) * WALL_RING_RADIUS;

		s.element = static_cast<ElementType>((i + offset) % ELEMENT_TYPE_COUNT);
		s.alive = true;
	}

	g_Active = true;
	g_Life = WALL_LIFE;
	g_Age = 0.0f;
}

void BossWall_Update(float delta_time)
{
	if (!g_Active) { return; }

	g_Age += delta_time;
	g_Life -= delta_time;

	if (g_Life <= 0.0f)
	{
		BossWall_ClearAll();
		return;
	}

	bool any_alive = false;
	for (const WallSegment& s : g_Segments)
	{
		if (s.alive) { any_alive = true; break; }
	}
	if (!any_alive) { BossWall_ClearAll(); }
}

// ============================================================================
// Movement
// ============================================================================
Vector2 BossWall_ResolvePosition(const Vector2& old_pos,
	const Vector2& new_pos,
	float radius)
{
	if (!g_Active) { return new_pos; }

	if (g_Age < WALL_RISE_TIME) { return new_pos; }

	Vector2 p = new_pos;
	const float min_dist = WALL_SEGMENT_RADIUS + radius;

	for (const WallSegment& s : g_Segments)
	{
		if (!s.alive) { continue; }

		const Vector2 d = p - s.pos;
		const float dist_sq = d.LengthSq();
		if (dist_sq >= min_dist * min_dist) { continue; }

		if (dist_sq <= 0.0001f)
		{
			p = s.pos + Vector2{ min_dist, 0.0f };
			continue;
		}

		p = s.pos + d * (min_dist / sqrtf(dist_sq));
	}

	return p;
}

int BossWall_GetSegmentCount() { return g_Active ? WALL_SEGMENTS : 0; }
bool BossWall_IsSegmentAlive(int index) { return ValidSegment(index); }
float BossWall_GetSegmentRadius(int index) { (void)index; return WALL_SEGMENT_RADIUS; }

Vector2 BossWall_GetSegmentPos(int index)
{
	return ValidSegment(index) ? g_Segments[index].pos : Vector2{ 0.0f, 0.0f };
}

ElementType BossWall_GetSegmentElement(int index)
{
	return ValidSegment(index) ? g_Segments[index].element : ELEMENT_FIRE;
}

bool BossWall_ApplyHit(int index, const HitInfo& hit)
{
	if (!ValidSegment(index)) { return false; }

	WallSegment& s = g_Segments[index];

	if ((hit.element_mask & ElementBit(s.element)) == 0) { return false; }

	s.alive = false;
	return true;
}

// ============================================================================
// Draw
// ============================================================================
void BossWall_Draw()
{
	if (!g_Active) { return; }

	const float rise = RiseScale();

	// flash faster and faster as the timer runs out
	float warn = 1.0f;
	if (g_Life < WALL_WARN_TIME)
	{
		const float t = 1.0f - (g_Life / WALL_WARN_TIME);
		warn = 0.45f + 0.55f * (0.5f + 0.5f * sinf(g_Age * (14.0f + 26.0f * t)));
	}

	for (const WallSegment& s : g_Segments)
	{
		if (!s.alive) { continue; }

		const XMFLOAT3 col = BossVisual_ElementColor(s.element);
		const float    r = WALL_SEGMENT_RADIUS * rise;

		DrawPrim_Circle(s.pos, r, col, 0.22f * warn);
		DrawPrim_Ring(s.pos, r, 4.0f, col, 0.90f * warn);
		DrawPrim_Ring(s.pos, r * 0.60f, 2.0f, col, 0.50f * warn);
	}
}