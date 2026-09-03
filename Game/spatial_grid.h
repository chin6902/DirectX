/*============================================================================
Contents   :  [spatial_grid.h]

Author     : Chin Qing You
LastUpdate : 2026/08/20
-----------------------------------------------------------------------------

============================================================================*/
#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H

#include "vector2.h"

inline constexpr float GRID_CELL_SIZE = 64.0f;

inline constexpr int GRID_QUERY_MAX = 256;

void SpatialGrid_Initialize();
void SpatialGrid_Finalize();

void SpatialGrid_Clear();
void SpatialGrid_Insert(int index, const Vector2& pos);

int SpatialGrid_Query(const Vector2& center, float radius, int* out, int out_max);

// Every pair within `radius` of each other, visited exactly once
// fn(a, b) is called with a < b.
void SpatialGrid_ForEachPair(float radius, void (*fn)(int a, int b));

int SpatialGrid_GetInsertedCount();

#endif