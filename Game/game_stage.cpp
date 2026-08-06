/*============================================================================
Contents   :  [game_stage.cpp]

Author     : Chin Qing You
LastUpdate : 2026/07/27
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>

#include "game_stage.h"
#include "config.h"
#include "texture.h"
#include "sprite.h"
#include "camera.h"
#include "collision_debug.h"

// ============================================================================
// Tilesheet layout (source texture, no spacing)
// ============================================================================
static int g_TextureId = -1;

static constexpr int TILE_SIZE = 18;   // texel size of one tile in the sheet
static constexpr int SHEET_COLS = 20;
static constexpr int SHEET_ROWS = 9;

// On-screen tile size (world pixels)
static constexpr float DRAW_TILE_SIZE = 64.0f;

// ============================================================================
// Level data — 3 screens each way (1600x900 screen / 64px tiles ~= 25x14)
// ============================================================================
static constexpr int MAP_WIDTH = 75;   // 4800 world px
static constexpr int MAP_HEIGHT = 42;   // 2688 world px

// tile ID = row * SHEET_COLS + col   (-1 = empty, draw nothing)
static int g_Map[MAP_HEIGHT][MAP_WIDTH];

// --- Debug-map tile IDs: swap these for IDs that look right in YOUR sheet ---
static constexpr int TILE_FLOOR = 0;
static constexpr int TILE_WALL = 22;

static constexpr float COLLISION_SKIN = 0.05f;

// ============================================================================
// Internal helpers
// ============================================================================
static void GenerateDebugMap();
static void DrawTile(int tileId, float screenX, float screenY);

static bool IsSolidTile(int tileId)
{
	// Grows into a switch / lookup table as the tileset grows.
	return tileId == TILE_WALL;
}

static bool IsSolidCell(int tx, int ty)
{
	// Out-of-bounds counts as solid: nothing may leave the map,
	// even if the wall ring is edited away later.
	if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT)
	{
		return true;
	}
	return IsSolidTile(g_Map[ty][tx]);
}

// ============================================================================
// Initialize / Finalize
// ============================================================================
void GameStage_Initialize()
{
	g_TextureId = Texture_Load(L"assets/textures/result.png");

	// Tier 1: generate the debug arena.
	// Tier 2 (later): if assets/maps/stage01.csv exists, load it instead.
	GenerateDebugMap();
}

void GameStage_Finalize()
{
	Texture_Release(g_TextureId);
}

static void GenerateDebugMap()
{
	// 1. Carpet everything with floor
	for (int y = 0; y < MAP_HEIGHT; ++y)
	{
		for (int x = 0; x < MAP_WIDTH; ++x)
		{
			g_Map[y][x] = TILE_FLOOR;
		}
	}

	// 2. Wall ring — these tiles ARE the stage boundary now
	for (int x = 0; x < MAP_WIDTH; ++x)
	{
		g_Map[0][x] = TILE_WALL;
		g_Map[MAP_HEIGHT - 1][x] = TILE_WALL;
	}
	for (int y = 0; y < MAP_HEIGHT; ++y)
	{
		g_Map[y][0] = TILE_WALL;
		g_Map[y][MAP_WIDTH - 1] = TILE_WALL;
	}

	// 3. Test fixtures in the open field
	//    2x2 pillar — circle around it, corner-slide against it
	for (int y = 10; y < 12; ++y)
	{
		for (int x = 15; x < 17; ++x) { g_Map[y][x] = TILE_WALL; }
	}
	//    Horizontal bar — slide along it left/right
	for (int x = 40; x < 46; ++x) { g_Map[20][x] = TILE_WALL; }
	//    Vertical bar — slide along it up/down
	for (int y = 25; y < 31; ++y) { g_Map[y][55] = TILE_WALL; }
}

// ============================================================================
// Drawing — cull to the camera rectangle, draw only visible tiles
// ============================================================================
static void DrawTile(int tileId, float screenX, float screenY)
{
	if (tileId < 0)
	{
		return;
	}

	const int col = tileId % SHEET_COLS;
	const int row = tileId / SHEET_COLS;

	Sprite_Draw(
		g_TextureId,
		screenX, screenY,
		DRAW_TILE_SIZE, DRAW_TILE_SIZE,
		static_cast<float>(col * TILE_SIZE),
		static_cast<float>(row * TILE_SIZE),
		TILE_SIZE, TILE_SIZE);
}

void GameStage_Draw()
{
	// Which tile range does the camera rectangle overlap?
	// The range IS the cull — no per-tile screen test needed.
	int startCol = static_cast<int>(Camera_GetX() / DRAW_TILE_SIZE);
	int startRow = static_cast<int>(Camera_GetY() / DRAW_TILE_SIZE);
	int endCol = startCol + static_cast<int>(SCREEN_WIDTH / DRAW_TILE_SIZE) + 2;
	int endRow = startRow + static_cast<int>(SCREEN_HEIGHT / DRAW_TILE_SIZE) + 2;

	startCol = std::max(startCol, 0);
	startRow = std::max(startRow, 0);
	endCol = std::min(endCol, MAP_WIDTH);
	endRow = std::min(endRow, MAP_HEIGHT);

	for (int my = startRow; my < endRow; ++my)
	{
		for (int mx = startCol; mx < endCol; ++mx)
		{
			const int tileId = g_Map[my][mx];
			if (tileId < 0)
			{
				continue;
			}

			DrawTile(tileId,
				Camera_WorldToScreenX(mx * DRAW_TILE_SIZE),
				Camera_WorldToScreenY(my * DRAW_TILE_SIZE));
		}
	}
}

// ============================================================================
// Queries
// ============================================================================
float GameStage_GetWidth() { return MAP_WIDTH * DRAW_TILE_SIZE; }
float GameStage_GetHeight() { return MAP_HEIGHT * DRAW_TILE_SIZE; }

bool GameStage_IsSolidAtWorld(float world_x, float world_y)
{
	return IsSolidCell(
		static_cast<int>(world_x / DRAW_TILE_SIZE),
		static_cast<int>(world_y / DRAW_TILE_SIZE));
}

// ============================================================================
// Collision resolve — circle vs solid tiles, one axis at a time
//
// The mover applies its FULL desired motion on one axis, we push it back
// out of any solid tile on that axis only, then repeat for the other
// axis. Because each axis is corrected independently, motion along a
// wall survives: pressing up-right into a wall on the right kills only
// the x part, and the mover slides upward. (Move-then-resolve.)
// ============================================================================

// Does a circle at (cx,cy) overlap the solid tile (tx,ty)?
// Standard circle-vs-AABB: clamp the centre into the rect to find the
// closest point, then compare that distance to the radius.
static bool CircleOverlapsTile(float cx, float cy, float radius, int tx, int ty)
{
	const float rect_min_x = tx * DRAW_TILE_SIZE;
	const float rect_min_y = ty * DRAW_TILE_SIZE;
	const float rect_max_x = rect_min_x + DRAW_TILE_SIZE;
	const float rect_max_y = rect_min_y + DRAW_TILE_SIZE;

	const float closest_x = std::clamp(cx, rect_min_x, rect_max_x);
	const float closest_y = std::clamp(cy, rect_min_y, rect_max_y);

	const float dx = cx - closest_x;
	const float dy = cy - closest_y;
	return (dx * dx + dy * dy) < (radius * radius);
}

static void GetOverlap(float cx, float cy, float radius, int tx, int ty,
	float& overlap_x, float& overlap_y)
{
	const float rect_min_x = tx * DRAW_TILE_SIZE;
	const float rect_min_y = ty * DRAW_TILE_SIZE;
	const float rect_max_x = rect_min_x + DRAW_TILE_SIZE;
	const float rect_max_y = rect_min_y + DRAW_TILE_SIZE;

	overlap_x = std::min(cx + radius, rect_max_x) - std::max(cx - radius, rect_min_x);
	overlap_y = std::min(cy + radius, rect_max_y) - std::max(cy - radius, rect_min_y);
}

// Resolve one axis. `pos` has already been moved on that axis;
// `moved_positive` is the sign of the motion (needed to know which
// tile edge to snap back to).
static void ResolveAxisX(Vector2& pos, float radius)
{
	// Only tiles the circle's bounding box can touch need checking —
	// same trick as the platformer CheckCollision, just per-axis.
	const int min_tx = static_cast<int>((pos.x - radius) / DRAW_TILE_SIZE);
	const int max_tx = static_cast<int>((pos.x + radius) / DRAW_TILE_SIZE);
	const int min_ty = static_cast<int>((pos.y - radius) / DRAW_TILE_SIZE);
	const int max_ty = static_cast<int>((pos.y + radius) / DRAW_TILE_SIZE);

	for (int ty = min_ty; ty <= max_ty; ++ty)
	{
		for (int tx = min_tx; tx <= max_tx; ++tx)
		{
			if (!IsSolidCell(tx, ty))
			{
				continue;
			}

			if (!CircleOverlapsTile(pos.x, pos.y, radius, tx, ty))
			{
				continue;
			}

			float overlap_x, overlap_y;
			GetOverlap(pos.x, pos.y, radius, tx, ty, overlap_x, overlap_y);
			if (overlap_y < overlap_x)
			{
				continue;   
			}

			const float tile_center_x = (tx + 0.5f) * DRAW_TILE_SIZE;
			if (pos.x < tile_center_x)
			{
				if (IsSolidCell(tx - 1, ty)) { continue; }   // left face is buried — not a real surface
				pos.x = tx * DRAW_TILE_SIZE - radius - COLLISION_SKIN;
			}
			else
			{
				if (IsSolidCell(tx + 1, ty)) { continue; }   // right face is buried
				pos.x = (tx + 1) * DRAW_TILE_SIZE + radius + COLLISION_SKIN;
			}
		}
	}
}

static void ResolveAxisY(Vector2& pos, float radius)
{
	const int min_tx = static_cast<int>((pos.x - radius) / DRAW_TILE_SIZE);
	const int max_tx = static_cast<int>((pos.x + radius) / DRAW_TILE_SIZE);
	const int min_ty = static_cast<int>((pos.y - radius) / DRAW_TILE_SIZE);
	const int max_ty = static_cast<int>((pos.y + radius) / DRAW_TILE_SIZE);

	for (int ty = min_ty; ty <= max_ty; ++ty)
	{
		for (int tx = min_tx; tx <= max_tx; ++tx)
		{
			if (!IsSolidCell(tx, ty))
			{
				continue;
			}

			if (!CircleOverlapsTile(pos.x, pos.y, radius, tx, ty))
			{
				continue;
			}

			float overlap_x, overlap_y;
			GetOverlap(pos.x, pos.y, radius, tx, ty, overlap_x, overlap_y);
			if (overlap_x < overlap_y)
			{
				continue;   // shallow vertical graze — the Y pass owns this one
			}

			const float tile_center_y = (ty + 0.5f) * DRAW_TILE_SIZE;
			if (pos.y < tile_center_y)
			{
				if (IsSolidCell(tx, ty - 1)) { continue; }   // top face is buried — not a real surface
				pos.y = ty * DRAW_TILE_SIZE - radius - COLLISION_SKIN;
			}
			else
			{
				if (IsSolidCell(tx, ty + 1)) { continue; }   // bottom face is buried
				pos.y = (ty + 1) * DRAW_TILE_SIZE + radius + COLLISION_SKIN;
			}
		}
	}
}

Vector2 GameStage_ResolvePosition(const Vector2& old_pos,
	const Vector2& new_pos,
	float radius)
{
	Vector2 pos = old_pos;

	pos.x = new_pos.x;
	ResolveAxisX(pos, radius);

	pos.y = new_pos.y;
	ResolveAxisY(pos, radius);

	return pos;
}

Vector2 GameStage_ClampPosition(const Vector2& pos, float radius)
{
	// Interior of the wall ring: one tile in from every edge.
	const float margin = DRAW_TILE_SIZE + radius;
	return {
		std::clamp(pos.x, margin, GameStage_GetWidth() - margin),
		std::clamp(pos.y, margin, GameStage_GetHeight() - margin)
	};
}

void GameStage_DebugDraw()
{
#ifdef _DEBUG
	// Same cull as GameStage_Draw — only visible tiles
	int startCol = std::max(static_cast<int>(Camera_GetX() / DRAW_TILE_SIZE), 0);
	int startRow = std::max(static_cast<int>(Camera_GetY() / DRAW_TILE_SIZE), 0);
	int endCol = std::min(startCol + static_cast<int>(SCREEN_WIDTH / DRAW_TILE_SIZE) + 2, MAP_WIDTH);
	int endRow = std::min(startRow + static_cast<int>(SCREEN_HEIGHT / DRAW_TILE_SIZE) + 2, MAP_HEIGHT);

	for (int my = startRow; my < endRow; ++my)
	{
		for (int mx = startCol; mx < endCol; ++mx)
		{
			if (!IsSolidTile(g_Map[my][mx]))
			{
				continue;
			}

			// One circle inscribed in each SOLID tile — magenta = "collision says wall"
			const float cx = (mx + 0.5f) * DRAW_TILE_SIZE;
			const float cy = (my + 0.5f) * DRAW_TILE_SIZE;
			Collision_Debug_Draw(
				{ { Camera_WorldToScreenX(cx), Camera_WorldToScreenY(cy) }, DRAW_TILE_SIZE * 0.5f },
				{ 1.0f, 0.0f, 1.0f });
		}
	}
#endif
}