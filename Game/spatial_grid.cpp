/*============================================================================
Contents   :  [spatial_grid.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/20
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>

#include "spatial_grid.h"
#include "game_stage.h"

static constexpr int GRID_COLS = 75;
static constexpr int GRID_ROWS = 42;
static constexpr int CELL_CAPACITY = 32;

struct Cell
{
	int count = 0;
	int items[CELL_CAPACITY]{};
};

static Cell g_Cells[GRID_ROWS][GRID_COLS];
static int  g_Inserted = 0;

static constexpr int POS_MAX = GRID_ROWS * GRID_COLS * 4;
static Vector2 g_Pos[POS_MAX];
static int     g_PosCount = 0;

void SpatialGrid_Initialize() { SpatialGrid_Clear(); }
void SpatialGrid_Finalize() { SpatialGrid_Clear(); }

int SpatialGrid_GetInsertedCount() { return g_Inserted; }

void SpatialGrid_Clear()
{
	for (int r = 0; r < GRID_ROWS; r++)
	{
		for (int c = 0; c < GRID_COLS; c++) { g_Cells[r][c].count = 0; }
	}
	g_Inserted = 0;
	g_PosCount = 0;
}

// World position -> cell
static void CellOf(const Vector2& pos, int& col, int& row)
{
	col = std::clamp(static_cast<int>(pos.x / GRID_CELL_SIZE), 0, GRID_COLS - 1);
	row = std::clamp(static_cast<int>(pos.y / GRID_CELL_SIZE), 0, GRID_ROWS - 1);
}

void SpatialGrid_Insert(int index, const Vector2& pos)
{
	if (index < 0 || index >= POS_MAX) { return; }

	int col, row;
	CellOf(pos, col, row);

	Cell& cell = g_Cells[row][col];
	if (cell.count >= CELL_CAPACITY) { return; }   

	cell.items[cell.count++] = index;

	g_Pos[index] = pos;
	g_PosCount = std::max(g_PosCount, index + 1);
	g_Inserted++;
}

// ----------------------------------------------------------------------------
// Query: visit only the cells the radius can reach, then distance-test.
// ----------------------------------------------------------------------------
int SpatialGrid_Query(const Vector2& center, float radius, int* out, int out_max)
{
	if (out == nullptr || out_max <= 0) { return 0; }

	const int reach = static_cast<int>(radius / GRID_CELL_SIZE) + 1;

	int c0, r0;
	CellOf(center, c0, r0);

	const int min_c = std::max(c0 - reach, 0);
	const int max_c = std::min(c0 + reach, GRID_COLS - 1);
	const int min_r = std::max(r0 - reach, 0);
	const int max_r = std::min(r0 + reach, GRID_ROWS - 1);

	const float r_sq = radius * radius;
	int found = 0;

	for (int r = min_r; r <= max_r; r++)
	{
		for (int c = min_c; c <= max_c; c++)
		{
			const Cell& cell = g_Cells[r][c];

			for (int i = 0; i < cell.count; i++)
			{
				const int idx = cell.items[i];
				if ((g_Pos[idx] - center).LengthSq() > r_sq) { continue; }

				out[found++] = idx;
				if (found >= out_max) { return found; }
			}
		}
	}
	return found;
}

static constexpr int NEIGHBOUR_DX[4] = { 1, -1,  0,  1 };
static constexpr int NEIGHBOUR_DY[4] = { 0,  1,  1,  1 };

void SpatialGrid_ForEachPair(float radius, void (*fn)(int a, int b))
{
	if (fn == nullptr) { return; }

	const float r_sq = radius * radius;

	for (int r = 0; r < GRID_ROWS; r++)
	{
		for (int c = 0; c < GRID_COLS; c++)
		{
			const Cell& cell = g_Cells[r][c];
			if (cell.count == 0) { continue; }

			// --- within this cell ---
			for (int i = 0; i < cell.count; i++)
			{
				for (int j = i + 1; j < cell.count; j++)
				{
					const int a = cell.items[i];
					const int b = cell.items[j];

					if ((g_Pos[a] - g_Pos[b]).LengthSq() > r_sq) { continue; }
					fn(std::min(a, b), std::max(a, b));
				}
			}

			// --- against the half-neighbourhood ---
			for (int n = 0; n < 4; n++)
			{
				const int nc = c + NEIGHBOUR_DX[n];
				const int nr = r + NEIGHBOUR_DY[n];

				if (nc < 0 || nc >= GRID_COLS || nr < 0 || nr >= GRID_ROWS) { continue; }

				const Cell& other = g_Cells[nr][nc];

				for (int i = 0; i < cell.count; i++)
				{
					for (int j = 0; j < other.count; j++)
					{
						const int a = cell.items[i];
						const int b = other.items[j];

						if ((g_Pos[a] - g_Pos[b]).LengthSq() > r_sq) { continue; }
						fn(std::min(a, b), std::max(a, b));
					}
				}
			}
		}
	}
}