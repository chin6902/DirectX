/*============================================================================
Contents   :  [enemy_visual.h]

Author     : Chin Qing You
LastUpdate : 2026/08/24
-----------------------------------------------------------------------------

============================================================================*/
#ifndef ENEMY_VISUAL_H
#define ENEMY_VISUAL_H

#include "game_enemy.h"
#include "sprite.h"
#include "vector2.h"
#include "game_audio.h"

enum EnemyClip
{
	ENEMY_CLIP_MOVE,
	ENEMY_CLIP_ATTACK,
	ENEMY_CLIP_COUNT,
};

enum EnemyFacing
{
	ENEMY_FACE_DOWN,
	ENEMY_FACE_UP,
	ENEMY_FACE_LEFT,
	ENEMY_FACE_RIGHT,
	ENEMY_FACE_COUNT,
};

void EnemyVisual_Initialize();
void EnemyVisual_Finalize();

int   EnemyVisual_GetFrameCount(EnemyType type, EnemyClip clip);
float EnemyVisual_GetFrameTime(EnemyType type, EnemyClip clip);

int   EnemyVisual_FacingFromDir(const Vector2& dir);

SoundId EnemyVisual_GetCycleSound(EnemyType type, EnemyClip clip);

void  EnemyVisual_Draw(EnemyType type, EnemyClip clip,
	int frame, int facing,
	const Vector2& world_pos, float draw_size,
	const SpriteDrawParams& params);

#endif