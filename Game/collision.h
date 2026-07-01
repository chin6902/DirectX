/*============================================================================
Contents   :  [collision.h]

Author     : Chin Qing You
LastUpdate : 2026/07/01
-----------------------------------------------------------------------------

============================================================================*/
#ifndef COLLISION_H
#define COLLISION_H

#include <DirectXMath.h>

struct CollisionCircle
{
	DirectX::XMFLOAT2 position;
	float radius;
};

bool Collision_IsOverlap(const CollisionCircle& a, const CollisionCircle& b);

// Spatial Grid Collision System
constexpr int GRID_CELL_SIZE = 64;
constexpr int GRID_WIDTH = 20;   // 1280 / 64
constexpr int GRID_HEIGHT = 17;  // 1080 / 64
constexpr int MAX_OBJECTS_PER_CELL = 32;

struct GridCell
{
	int objectIndices[MAX_OBJECTS_PER_CELL];
	int objectCount;
};

// Get grid cell coordinates from world position
void Grid_GetCellCoordinates(float x, float y, int& gridX, int& gridY);

// Clear the grid (call at start of update)
void Grid_Clear();

// Add object to grid at given world position
void Grid_AddObject(int objectIndex, float x, float y);

// Query which objects are in or near a specific position
void Grid_QueryNearby(float x, float y, int*& outIndices, int& outCount, int maxResults);

#endif
