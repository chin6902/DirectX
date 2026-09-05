/*============================================================================
Contents   :  [boss_wall.h]

Author     : Chin Qing You
LastUpdate : 2026/09/04
-----------------------------------------------------------------------------

============================================================================*/
#ifndef BOSS_WALL_H
#define BOSS_WALL_H

#include "vector2.h"
#include "game_enemy.h"   

inline constexpr int WALL_SEGMENT_MAX = 16;

void BossWall_Initialize();
void BossWall_Finalize();
void BossWall_Update(float delta_time);
void BossWall_Draw();

void BossWall_SpawnRing(const Vector2& center);
void BossWall_ClearAll();
bool BossWall_IsActive();

Vector2 BossWall_ResolvePosition(const Vector2& old_pos,
	const Vector2& new_pos,
	float radius);

int         BossWall_GetSegmentCount();
bool        BossWall_IsSegmentAlive(int index);
Vector2     BossWall_GetSegmentPos(int index);
float       BossWall_GetSegmentRadius(int index);
ElementType BossWall_GetSegmentElement(int index);

bool BossWall_ApplyHit(int index, const HitInfo& hit);

#endif