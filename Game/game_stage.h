/*============================================================================
Contents   :  [game_stage.h]

Author     : Chin Qing You
LastUpdate : 2026/07/27
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_STAGE_H
#define GAME_STAGE_H

#include "vector2.h"

void GameStage_Initialize();
void GameStage_Finalize();
void GameStage_Draw();

float GameStage_GetWidth();
float GameStage_GetHeight();

bool GameStage_IsSolidAtWorld(float world_x, float world_y);

// old_pos : position at the start of the frame 
// new_pos : where the mover WANTS to be after this frame
// radius  : collider radius
// Returns the nearest walkable position, resolving X then Y so movers
Vector2 GameStage_ResolvePosition(const Vector2& old_pos,
    const Vector2& new_pos,
    float radius);

Vector2 GameStage_ClampPosition(const Vector2& pos, float radius);

bool GameStage_TryGetObstacleSpawnPoint(float& out_x, float& out_y);

void GameStage_DebugDraw();

#endif