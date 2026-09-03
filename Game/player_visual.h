/*============================================================================
Contents   :  [player_visual.h]

Author     : Chin Qing You
LastUpdate : 2026/08/24
-----------------------------------------------------------------------------

============================================================================*/
#ifndef PLAYER_VISUAL_H
#define PLAYER_VISUAL_H

#include "sprite.h"
#include "vector2.h"

enum PlayerClip
{
	PLAYER_CLIP_IDLE,
	PLAYER_CLIP_RUN,
	PLAYER_CLIP_DEATH,
	PLAYER_CLIP_COUNT,
};

enum PlayerFacing
{
	PLAYER_FACE_DOWN,
	PLAYER_FACE_UP,
	PLAYER_FACE_LEFT,
	PLAYER_FACE_RIGHT,
	PLAYER_FACE_COUNT,
};

void PlayerVisual_Initialize();
void PlayerVisual_Finalize();

int   PlayerVisual_GetFrameCount(PlayerClip clip);
float PlayerVisual_GetFrameTime(PlayerClip clip);

bool  PlayerVisual_IsLooping(PlayerClip clip);

int   PlayerVisual_FacingFromDir(const Vector2& dir);

int   PlayerVisual_FacingSticky(const Vector2& dir, int current_facing);

void  PlayerVisual_Draw(PlayerClip clip, int frame, int facing,
	const Vector2& world_pos, float draw_size,
	const SpriteDrawParams& params);

#endif