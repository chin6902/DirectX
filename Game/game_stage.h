/*============================================================================
Contents   :  [game_stage.h]

Author     : Chin Qing You
LastUpdate : 2026/07/27
-----------------------------------------------------------------------------
Top-down tilemap stage. Owns the map data, tile drawing, stage size,
and the ONE gateway for "can I stand here?" (ResolvePosition).

Internals will change (CSV loader, Tiled export, per-level maps);
this interface should not.
============================================================================*/
#ifndef GAME_STAGE_H
#define GAME_STAGE_H

#include "vector2.h"

void GameStage_Initialize();
void GameStage_Finalize();
void GameStage_Draw();

// Stage size in world pixels (map origin is world (0,0), top-left)
float GameStage_GetWidth();
float GameStage_GetHeight();

// Is the tile under this world position solid?
bool GameStage_IsSolidAtWorld(float world_x, float world_y);

// Move-then-resolve gateway for a circle collider.
//   old_pos : position at the start of the frame (known legal)
//   new_pos : where the mover WANTS to be after this frame
//   radius  : collider radius
// Returns the nearest legal position, resolving X then Y so movers
// slide along walls instead of sticking to them.
Vector2 GameStage_ResolvePosition(const Vector2& old_pos,
    const Vector2& new_pos,
    float radius);

// Simple interior clamp (inside the outer wall ring). For systems that
// just need "a legal point roughly here" (e.g. spawner), not sliding.
Vector2 GameStage_ClampPosition(const Vector2& pos, float radius);

void GameStage_DebugDraw();

#endif