/*============================================================================
Contents   :  [flow_field.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/21
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>

#include "flow_field.h"
#include "game_stage.h"
#include "camera.h"
#include "config.h"
#include "draw_primitives.h"

static constexpr int FIELD_COLS = 75;
static constexpr int FIELD_ROWS = 42;
static constexpr float TILE_SIZE = 64.0f;

static constexpr int UNREACHABLE = -1;

static int g_Distance[FIELD_ROWS][FIELD_COLS]{};
static float g_DirX[FIELD_ROWS][FIELD_COLS]{}; 
static float g_DirY[FIELD_ROWS][FIELD_COLS]{};

static int g_QueueX[FIELD_COLS * FIELD_ROWS];
static int g_QueueY[FIELD_COLS * FIELD_ROWS];

static int g_LastCol = -9999;
static int g_LastRow = -9999;
static bool g_Valid;

// 8 neighbour
static constexpr int NX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
static constexpr int NY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

static bool InBounds(int c, int r)
{
	return c >= 0 && c < FIELD_COLS && r >= 0 && r < FIELD_ROWS;
}

static bool Walkable(int c, int r)
{
	if (!InBounds(c, r)) { return false; }

	return !GameStage_IsSolidAtWorld
	(
		c * TILE_SIZE + TILE_SIZE * 0.5f,
		r * TILE_SIZE + TILE_SIZE * 0.5f
	);
}

void FlowField_Initialize()
{
	g_LastCol = -9999;
	g_LastRow = -9999;
	g_Valid = false;
}

void FlowField_Finalize()
{
	g_Valid = false;
}

static void Rebuild(int start_c, int start_r)
{
	for (int r = 0; r < FIELD_ROWS; r++)
	{
		for (int c = 0; c < FIELD_COLS; c++)
		{
			g_Distance[r][c] = UNREACHABLE;
			g_DirX[r][c] = 0.0f;
			g_DirY[r][c] = 0.0f;
		}
	}

	if (!Walkable(start_c, start_r))
	{
		g_Valid = false;
		return;
	}

	int head = 0;
	int tail = 0;

	g_Distance[start_r][start_c] = 0;
	g_QueueX[tail] = start_c;
	g_QueueY[tail] = start_r;
	tail++;

	while (head < tail)
	{
		const int cx = g_QueueX[head];
		const int cy = g_QueueY[head];
		head++;

		const int next_dist = g_Distance[cy][cx] + 1;

		for (int n = 0; n < 8; n++)
		{
			const int nx = cx + NX[n];
			const int ny = cy + NY[n];

			if (!Walkable(nx, ny)) { continue; }
			if (g_Distance[ny][nx] != UNREACHABLE) { continue; } // already shortest

			// diagonal: only if both orthogonal neighbours are open
			if (NX[n] != 0 && NY[n] != 0)
			{
				if (!Walkable(cx + NX[n], cy) || !Walkable(cx, cy + NY[n])) { continue; }
			}

			g_Distance[ny][nx] = next_dist;

			const float dx = static_cast<float>(cx - nx);
			const float dy = static_cast<float>(cy - ny);
			const float len = sqrt(dx * dx + dy * dy);

			g_DirX[ny][nx] = dx / len;
			g_DirY[ny][nx] = dy / len;

			g_QueueX[tail] = nx;
			g_QueueY[tail] = ny;
			tail++;
		}
	}

	g_Valid = true;
}

void FlowField_Update(const Vector2& target_world)
{
	const int c = static_cast<int>(target_world.x / TILE_SIZE);
	const int r = static_cast<int>(target_world.y / TILE_SIZE);

	if (c == g_LastCol && r == g_LastRow && g_Valid) { return; }

	g_LastCol = c;
	g_LastRow = r;
	Rebuild(c, r);
}

static bool CellOf(const Vector2& pos, int& c, int& r)
{
	c = static_cast<int>(pos.x / TILE_SIZE);
	r = static_cast<int>(pos.y / TILE_SIZE);
	return InBounds(c, r);
}

bool FlowField_HasRoute(const Vector2& world_pos)
{
	int c, r;
	if (!g_Valid || !CellOf(world_pos, c, r)) { return false; }
	return g_Distance[r][c] > 0;
}

Vector2 FlowField_GetDirection(const Vector2& world_pos)
{
	int c, r;
	if (!g_Valid || !CellOf(world_pos, c, r)) { return { 0.0f, 0.0f }; }
	if (g_Distance[r][c] <= 0) { return { 0.0f, 0.0f }; }

	return { g_DirX[r][c], g_DirY[r][c] };
}

void FlowField_DebugDraw()
{
#ifdef _DEBUG
	if (!g_Valid) { return; }

	const int c0 = std::max(static_cast<int>(Camera_GetX() / TILE_SIZE), 0);
	const int r0 = std::max(static_cast<int>(Camera_GetY() / TILE_SIZE), 0);
	const int c1 = std::min(c0 + static_cast<int>(SCREEN_WIDTH / TILE_SIZE) + 2, FIELD_COLS);
	const int r1 = std::min(r0 + static_cast<int>(SCREEN_HEIGHT / TILE_SIZE) + 2, FIELD_ROWS);

	for (int r = r0; r < r1; r++)
	{
		for (int c = c0; c < c1; c++)
		{
			if (g_Distance[r][c] <= 0) { continue; }

			const Vector2 centre{ (c + 0.5f) * TILE_SIZE, (r + 0.5f) * TILE_SIZE };
			const Vector2 dir{ g_DirX[r][c], g_DirY[r][c] };

			DrawPrim_Line(centre, centre + dir * 22.0f, 2.0f,
				{ 0.35f, 1.0f, 0.45f }, 0.6f);
		}
	}
#endif
}
