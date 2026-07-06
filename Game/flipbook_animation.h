/*============================================================================
Contents   :  [flipbook_animation.h]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#pragma once
#include "sprite.h"

// ============================================================================
// AnimPlayMode
//
// Controls how an animation advances through its frame range.
//
//   LOOP      — plays frame_start → frame_end, wraps back to frame_start.
//               Default behaviour; same as the original system.
//
//   ONE_SHOT  — plays frame_start → frame_end once, then freezes on frame_end.
//               Use for death, hit-reaction, or any effect that plays once.
//               Check FlipBookAnimation_IsFinished() to know when it's done.
//
//   FREEZE    — stays on the current frame and never advances.
//               Use to pause mid-animation or to hold a specific pose.
// ============================================================================
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

// ============================================================================
// Per-animation controls
//
// These can be called at any time after Create().
//
// FlipBookAnimation_SetRange   — restrict playback to [frame_start, frame_end].
//   e.g. SetRange(id, 4, 7) loops only frames 4,5,6,7.
//   Resets is_finished and clamps the current frame into the new range.
//
// FlipBookAnimation_SetMode    — change LOOP / ONE_SHOT / FREEZE.
//   Resets is_finished so ONE_SHOT can be replayed after switching back to it.
//
// FlipBookAnimation_SetFrame   — jump immediately to a specific frame.
//   Also resets the frame timer so the new frame gets its full display time.
//
// FlipBookAnimation_IsFinished — returns true after a ONE_SHOT completes.
//   Always false for LOOP and FREEZE.
// ============================================================================
void FlipBookAnimation_SetRange(int animation_id, int frame_start, int frame_end);
void FlipBookAnimation_SetMode(int animation_id, AnimPlayMode mode);
void FlipBookAnimation_SetFrame(int animation_id, int frame);
bool FlipBookAnimation_IsFinished(int animation_id);

void FlipBookAnimation_Update(float delta_time);

void FlipBookAnimation_Draw(int animation_id, float x, float y , float width, float height);
void FlipBookAnimation_Draw(int animation_id, float x, float y, float width, float height, const SpriteDrawParams& params);