/*============================================================================
Contents   :  [flipbook_animation.h]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#pragma once
#include "sprite.h"

enum class AnimPlayMode
{
    LOOP,
    ONE_SHOT,
    FREEZE,
};

void FlipBookAnimation_Initialize();
void FlipBookAnimation_Finalize();

int FlipBookAnimation_Create(
    int   texture_id,
    int   pattern_width,
    int   pattern_height,
    int   pattern_count_max,
    int   pattern_column_count_max,
    float pattern_update_time
);

void FlipBookAnimation_Destroy(int animation_id);

void FlipBookAnimation_SetRange(int animation_id, int frame_start, int frame_end);
void FlipBookAnimation_SetMode(int animation_id, AnimPlayMode mode);
void FlipBookAnimation_SetFrame(int animation_id, int frame);
bool FlipBookAnimation_IsFinished(int animation_id);

void FlipBookAnimation_Update(float delta_time);

void FlipBookAnimation_Draw(int animation_id, float x, float y , float width, float height);
void FlipBookAnimation_Draw(int animation_id, float x, float y, float width, float height, const SpriteDrawParams& params);