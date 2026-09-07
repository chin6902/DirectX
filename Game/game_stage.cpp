/*============================================================================
Contents   :  [game_stage.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/20
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "game_stage.h"
#include "config.h"
#include "texture.h"
#include "sprite.h"
#include "camera.h"
#include "collision_debug.h"

static int g_TextureBg = -1;   
static int g_TextureStone = -1;   

static constexpr int STONE_TEX_W = 472;
static constexpr int STONE_TEX_H = 474;

static constexpr float DRAW_TILE_SIZE = 64.0f;   

static constexpr int MAP_WIDTH = 75;   
static constexpr int MAP_HEIGHT = 42;   

static constexpr int TILE_FLOOR = 0;
static constexpr int TILE_WALL = 1;

static int g_Map[MAP_HEIGHT][MAP_WIDTH];

static constexpr float COLLISION_SKIN = 0.05f;

static constexpr int QUADRANT_MARGIN = 5;    
static constexpr int OBSTACLE_SPAN = 5;    

enum ObstacleShape
{
	SHAPE_BLOCK,
	SHAPE_L,
	SHAPE_PLUS,
	SHAPE_COUNT,
};

static void GenerateMap();

static bool IsSolidTile(int tileId)
{
	return tileId == TILE_WALL;
}

static bool IsSolidCell(int tx, int ty)
{
	if (tx < 0 || tx >= MAP_WIDTH || ty < 0 || ty >= MAP_HEIGHT)
	{
		return true;
	}
	return IsSolidTile(g_Map[ty][tx]);
}

void GameStage_Initialize()
{
	g_TextureBg = Texture_Load(L"assets/textures/bg1.png", false);
	g_TextureStone = Texture_Load(L"assets/textures/stone.png");

	GenerateMap();
}

void GameStage_Finalize()
{
	Texture_Release(g_TextureBg);
	Texture_Release(g_TextureStone);
}

static void SetWall(int tx, int ty)
{
	if (tx < 1 || tx >= MAP_WIDTH - 1) { return; }
	if (ty < 1 || ty >= MAP_HEIGHT - 1) { return; }

	g_Map[ty][tx] = TILE_WALL;
}

static void PlaceObstacle(int min_x, int min_y, int max_x, int max_y)
{
	const int span_x = std::max(1, max_x - min_x - OBSTACLE_SPAN);
	const int span_y = std::max(1, max_y - min_y - OBSTACLE_SPAN);

	const int x = min_x + rand() % span_x;
	const int y = min_y + rand() % span_y;

	const ObstacleShape shape = static_cast<ObstacleShape>(rand() % SHAPE_COUNT);

	switch (shape)
	{
	case SHAPE_BLOCK:
	{
		const int w = 2 + rand() % 3;      
		const int h = 2 + rand() % 3;

		for (int ty = 0; ty < h; ++ty)
		{
			for (int tx = 0; tx < w; ++tx) { SetWall(x + tx, y + ty); }
		}
		break;
	}

	case SHAPE_L:
	{
		const int arm = 3 + rand() % 2;   
		const int turn = rand() % 4;       // which corner the L opens toward

		for (int i = 0; i < arm; ++i)
		{
			switch (turn)
			{
			case 0: 
				SetWall(x + i, y);      
				SetWall(x, y + i);      
				break;
			case 1:
				SetWall(x + i, y);      
				SetWall(x + arm - 1, y + i); 
				break;
			case 2: 
				SetWall(x + i, y + arm - 1); 
				SetWall(x, y + i);   
				break;
			default: 
				SetWall(x + i, y + arm - 1);
				SetWall(x + arm - 1, y + i); 
				break;
			}
		}
		break;
	}

	case SHAPE_PLUS:
	default:
	{
		const int arm = 1 + rand() % 2;    // arm length either side of centre
		const int cx = x + OBSTACLE_SPAN / 2;
		const int cy = y + OBSTACLE_SPAN / 2;

		SetWall(cx, cy);
		for (int i = 1; i <= arm; ++i)
		{
			SetWall(cx + i, cy);
			SetWall(cx - i, cy);
			SetWall(cx, cy + i);
			SetWall(cx, cy - i);
		}
		break;
	}
	}
}

static void GenerateMap()
{
	for (int y = 0; y < MAP_HEIGHT; ++y)
	{
		for (int x = 0; x < MAP_WIDTH; ++x)
		{
			g_Map[y][x] = TILE_FLOOR;
		}
	}

	// --- wall ring: these tiles ARE the stage boundary ---
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

	// --- one obstacle per quadrant ---
	const int mid_x = MAP_WIDTH / 2;
	const int mid_y = MAP_HEIGHT / 2;
	const int m = QUADRANT_MARGIN;

	PlaceObstacle(m, m, mid_x - m, mid_y - m);							// top-left
	PlaceObstacle(mid_x + m, m, MAP_WIDTH - m, mid_y - m);				// top-right
	PlaceObstacle(m, mid_y + m, mid_x - m, MAP_HEIGHT - m);				// bottom-left
	PlaceObstacle(mid_x + m, mid_y + m, MAP_WIDTH - m, MAP_HEIGHT - m); // bottom-right
}

// ============================================================================
// Draw
// ============================================================================
void GameStage_Draw()
{
	// --- floor
	Sprite_Draw(
		g_TextureBg,
		0.0f, 0.0f,
		static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT),
		Camera_GetX(), Camera_GetY(),
		static_cast<int>(SCREEN_WIDTH), static_cast<int>(SCREEN_HEIGHT));

	// --- walls
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
			if (!IsSolidTile(g_Map[my][mx])) { continue; }

			Sprite_Draw(
				g_TextureStone,
				Camera_WorldToScreenX(mx * DRAW_TILE_SIZE),
				Camera_WorldToScreenY(my * DRAW_TILE_SIZE),
				DRAW_TILE_SIZE, DRAW_TILE_SIZE,
				0.0f, 0.0f,
				STONE_TEX_W, STONE_TEX_H);
		}
	}
}

float GameStage_GetWidth() { return MAP_WIDTH * DRAW_TILE_SIZE; }
float GameStage_GetHeight() { return MAP_HEIGHT * DRAW_TILE_SIZE; }

bool GameStage_IsSolidAtWorld(float world_x, float world_y)
{
	return IsSolidCell(
		static_cast<int>(world_x / DRAW_TILE_SIZE),
		static_cast<int>(world_y / DRAW_TILE_SIZE));
}

// ============================================================================
// Collision resolve
// ============================================================================
static bool CircleOverlapsTile(float cx, float cy, float radius, int tx, int ty)
{
	const float rect_min_x = tx * DRAW_TILE_SIZE;
	const float rect_min_y = ty * DRAW_TILE_SIZE;
	const float rect_max_x = rect_min_x + DRAW_TILE_SIZE;
	const float rect_max_y = rect_min_y + DRAW_TILE_SIZE;

	// circle vs AABB: clamp the centre into the rect for the closest point
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

static void ResolveAxisX(Vector2& pos, float radius)
{
	const int min_tx = static_cast<int>((pos.x - radius) / DRAW_TILE_SIZE);
	const int max_tx = static_cast<int>((pos.x + radius) / DRAW_TILE_SIZE);
	const int min_ty = static_cast<int>((pos.y - radius) / DRAW_TILE_SIZE);
	const int max_ty = static_cast<int>((pos.y + radius) / DRAW_TILE_SIZE);

	for (int ty = min_ty; ty <= max_ty; ++ty)
	{
		for (int tx = min_tx; tx <= max_tx; ++tx)
		{
			if (!IsSolidCell(tx, ty)) { continue; }
			if (!CircleOverlapsTile(pos.x, pos.y, radius, tx, ty)) { continue; }

			float overlap_x, overlap_y;
			GetOverlap(pos.x, pos.y, radius, tx, ty, overlap_x, overlap_y);
			if (overlap_y < overlap_x) { continue; }   

			const float tile_center_x = (tx + 0.5f) * DRAW_TILE_SIZE;
			if (pos.x < tile_center_x)
			{
				if (IsSolidCell(tx - 1, ty)) { continue; }   
				pos.x = tx * DRAW_TILE_SIZE - radius - COLLISION_SKIN;
			}
			else
			{
				if (IsSolidCell(tx + 1, ty)) { continue; }
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
			if (!IsSolidCell(tx, ty)) { continue; }
			if (!CircleOverlapsTile(pos.x, pos.y, radius, tx, ty)) { continue; }

			float overlap_x, overlap_y;
			GetOverlap(pos.x, pos.y, radius, tx, ty, overlap_x, overlap_y);
			if (overlap_x < overlap_y) { continue; }   

			const float tile_center_y = (ty + 0.5f) * DRAW_TILE_SIZE;
			if (pos.y < tile_center_y)
			{
				if (IsSolidCell(tx, ty - 1)) { continue; }  
				pos.y = ty * DRAW_TILE_SIZE - radius - COLLISION_SKIN;
			}
			else
			{
				if (IsSolidCell(tx, ty + 1)) { continue; }
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
	const float margin = DRAW_TILE_SIZE + radius;
	return {
		std::clamp(pos.x, margin, GameStage_GetWidth() - margin),
		std::clamp(pos.y, margin, GameStage_GetHeight() - margin)
	};
}

bool GameStage_TryGetObstacleSpawnPoint(float& out_x, float& out_y)
{
	for (int attempt = 0; attempt < 64; ++attempt)
	{
		const int tx = 2 + rand() % (MAP_WIDTH - 4);
		const int ty = 2 + rand() % (MAP_HEIGHT - 4);

		if (IsSolidTile(g_Map[ty][tx])) { continue; }   // must stand on floor

		// accept only if a 4-neighbour is an interior obstacle 
		const int nx[4] = { tx + 1, tx - 1, tx,     tx };
		const int ny[4] = { ty,     ty,     ty + 1, ty - 1 };
		bool near_obstacle = false;
		for (int i = 0; i < 4; ++i)
		{
			const int cx = nx[i], cy = ny[i];
			if (cx <= 0 || cx >= MAP_WIDTH - 1 || cy <= 0 || cy >= MAP_HEIGHT - 1) { continue; }
			if (IsSolidTile(g_Map[cy][cx])) { near_obstacle = true; break; }
		}
		if (!near_obstacle) { continue; }

		out_x = (tx + 0.5f) * DRAW_TILE_SIZE;
		out_y = (ty + 0.5f) * DRAW_TILE_SIZE;
		return true;
	}
	return false;
}

void GameStage_DebugDraw()
{
#ifdef _DEBUG
	int startCol = std::max(static_cast<int>(Camera_GetX() / DRAW_TILE_SIZE), 0);
	int startRow = std::max(static_cast<int>(Camera_GetY() / DRAW_TILE_SIZE), 0);
	int endCol = std::min(startCol + static_cast<int>(SCREEN_WIDTH / DRAW_TILE_SIZE) + 2, MAP_WIDTH);
	int endRow = std::min(startRow + static_cast<int>(SCREEN_HEIGHT / DRAW_TILE_SIZE) + 2, MAP_HEIGHT);

	for (int my = startRow; my < endRow; ++my)
	{
		for (int mx = startCol; mx < endCol; ++mx)
		{
			if (!IsSolidTile(g_Map[my][mx])) { continue; }

			const float cx = (mx + 0.5f) * DRAW_TILE_SIZE;
			const float cy = (my + 0.5f) * DRAW_TILE_SIZE;

			Collision_Debug_Draw(
				{ { Camera_WorldToScreenX(cx), Camera_WorldToScreenY(cy) },
				  DRAW_TILE_SIZE * 0.5f },
				{ 1.0f, 0.0f, 1.0f });
		}
	}
#endif
}