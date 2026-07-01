/*============================================================================
Contents   :  [flipbook_animation.cpp]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>  

#include "flipbook_animation.h"
#include "sprite.h"

struct FlipBookAnimation
{
    int   texture_id;
    int   pattern_width;
    int   pattern_height;
    int   pattern_count_max;        // total frames in sheet (0 = slot unused)
    int   pattern_column_count_max;
    float pattern_update_time;      // seconds per frame

    int   pattern_current_count;    // which frame is showing right now
    float pattern_accumulated_time;

    // Playback range — defaults to [0, pattern_count_max - 1]
    int          frame_start;
    int          frame_end;

    AnimPlayMode mode;              // LOOP / ONE_SHOT / FREEZE
    bool         is_finished;       // true when ONE_SHOT reaches frame_end
};

static constexpr int ANIMATION_MAX{ 128 };
static FlipBookAnimation g_Animations[ANIMATION_MAX]{};

void FlipBookAnimation_Initialize()
{
    for (FlipBookAnimation& a : g_Animations)
    {
        a.pattern_count_max = 0;
    }
}

void FlipBookAnimation_Finalize()
{

}

int FlipBookAnimation_Create(
    int   texture_id,
    int   pattern_width,
    int   pattern_height,
    int   pattern_count_max,
    int   pattern_column_count_max,
    float pattern_update_time)
{
    for (int i = 0; i < ANIMATION_MAX; i++)
    {
        if (g_Animations[i].pattern_count_max != 0)
        {
            continue;
        }

        g_Animations[i].texture_id = texture_id;
        g_Animations[i].pattern_width = pattern_width;
        g_Animations[i].pattern_height = pattern_height;
        g_Animations[i].pattern_count_max = pattern_count_max;
        g_Animations[i].pattern_column_count_max = pattern_column_count_max;
        g_Animations[i].pattern_update_time = pattern_update_time;
        g_Animations[i].pattern_current_count = 0;
        g_Animations[i].pattern_accumulated_time = 0.0f;

        // Defaults: play all frames, looping
        g_Animations[i].frame_start = 0;
        g_Animations[i].frame_end = pattern_count_max - 1;
        g_Animations[i].mode = AnimPlayMode::LOOP;
        g_Animations[i].is_finished = false;

        return i;
    }
    return -1;
}

// ============================================================================
// Destroy
// ============================================================================
void FlipBookAnimation_Destroy(int animation_id)
{
    if (animation_id < 0 || animation_id >= ANIMATION_MAX)
    {
        return;
    }

    g_Animations[animation_id].pattern_count_max = 0;
}

// ============================================================================
// SetRange
//
// Restricts playback to [frame_start, frame_end] within the sheet.
// The current frame is clamped into the new range so nothing glitches.
//
// Example — run only frames 4-7, then switch back to all frames:
//   FlipBookAnimation_SetRange(id, 4, 7);   // plays 4,5,6,7 on loop
//   FlipBookAnimation_SetRange(id, 0, 9);   // back to full loop
// ============================================================================
void FlipBookAnimation_SetRange(int animation_id, int frame_start, int frame_end)
{
    if (animation_id < 0 || animation_id >= ANIMATION_MAX)
    {
        return;
    }
    FlipBookAnimation& a = g_Animations[animation_id];
    if (a.pattern_count_max == 0)
    {
        return;
    }

    int max_frame = a.pattern_count_max - 1;
    a.frame_start = std::clamp(frame_start, 0, max_frame);
    a.frame_end = std::clamp(frame_end, a.frame_start, max_frame);

    // Keep the current frame in range rather than snapping to start
    a.pattern_current_count = std::clamp(a.pattern_current_count, a.frame_start, a.frame_end);
    a.is_finished = false;
}

// ============================================================================
// SetMode
//
// Switches the play mode.  Resets is_finished so ONE_SHOT can be re-triggered.
//
// Example — play a hit animation once, then hold the last frame:
//   FlipBookAnimation_SetRange(id, 5, 9);
//   FlipBookAnimation_SetMode (id, AnimPlayMode::ONE_SHOT);
//   // ... later, once IsFinished() returns true, the last frame is held automatically
//
// Example — freeze on whatever frame is showing right now:
//   FlipBookAnimation_SetMode(id, AnimPlayMode::FREEZE);
// ============================================================================
void FlipBookAnimation_SetMode(int animation_id, AnimPlayMode mode)
{
    if (animation_id < 0 || animation_id >= ANIMATION_MAX)
    {
        return;
    }
    FlipBookAnimation& a = g_Animations[animation_id];
    if (a.pattern_count_max == 0) 
    {
        return;
    }

    a.mode = mode;
    a.is_finished = false;
}

// ============================================================================
// SetFrame
//
// Jumps to a specific frame immediately.  Also resets the frame timer so the
// new frame gets its full display time before advancing.
//
// Example — start from frame 3 instead of 0:
//   FlipBookAnimation_SetFrame(id, 3);
//
// Example — loop just frame 5 (freeze-via-range):
//   FlipBookAnimation_SetRange(id, 5, 5);   // range of one frame
// ============================================================================
void FlipBookAnimation_SetFrame(int animation_id, int frame)
{
    if (animation_id < 0 || animation_id >= ANIMATION_MAX)
    {
        return;
    }
    FlipBookAnimation& a = g_Animations[animation_id];
    if (a.pattern_count_max == 0)
    {
        return;
    }   

    a.pattern_current_count = std::clamp(frame, 0, a.pattern_count_max - 1);
    a.pattern_accumulated_time = 0.0f;
    a.is_finished = false;
}

// ============================================================================
// IsFinished
//
// Returns true when a ONE_SHOT animation has played through to frame_end.
// Always returns false for LOOP and FREEZE.
//
// Typical use — trigger something when an animation ends:
//   if (FlipBookAnimation_IsFinished(g_HitAnimId))
//   {
//       // start next animation, destroy object, etc.
//   }
// ============================================================================
bool FlipBookAnimation_IsFinished(int animation_id)
{
    if (animation_id < 0 || animation_id >= ANIMATION_MAX)
    {
        return false;
    }

    return g_Animations[animation_id].is_finished;
}

void FlipBookAnimation_Update(float delta_time)
{
    for (int i = 0; i < ANIMATION_MAX; i++)
    {
        FlipBookAnimation& a = g_Animations[i];
        if (a.pattern_count_max == 0)
        { 
            continue; 
        }
        if (a.mode == AnimPlayMode::FREEZE) 
        { 
            continue;
        }
        if (a.is_finished)            
        { 
            continue; 
        }

        a.pattern_accumulated_time += delta_time;
        if (a.pattern_accumulated_time < a.pattern_update_time) 
        { 
            continue; 
        }

        a.pattern_accumulated_time -= a.pattern_update_time;
        a.pattern_current_count++;

        switch (a.mode)
        {
        case AnimPlayMode::LOOP:
            // Wrap back to the start of the range
            if (a.pattern_current_count > a.frame_end)
            {
                a.pattern_current_count = a.frame_start;
            }
            break;

        case AnimPlayMode::ONE_SHOT:
            // Clamp to last frame and mark done
            if (a.pattern_current_count > a.frame_end)
            {
                a.pattern_current_count = a.frame_end;
                a.is_finished = true;
            }
            break;

        default:
            break;
        }
    }
}

// ============================================================================
// Draw
// ============================================================================
void FlipBookAnimation_Draw(int animation_id, float x, float y)
{
    FlipBookAnimation_Draw(animation_id, x, y, SpriteDrawParams{});
}

void FlipBookAnimation_Draw(int animation_id, float x, float y, const SpriteDrawParams& params)
{
    if (animation_id < 0 || animation_id >= ANIMATION_MAX)
    {
        return;
    }

    const FlipBookAnimation& anim = g_Animations[animation_id];
    if (anim.pattern_count_max == 0)
    {
        return;
    }

    int col = anim.pattern_current_count % anim.pattern_column_count_max;
    int row = anim.pattern_current_count / anim.pattern_column_count_max;

    float texture_x = static_cast<float>(col * anim.pattern_width);
    float texture_y = static_cast<float>(row * anim.pattern_height);

    Sprite_Draw(
        anim.texture_id,
        x, y,
        static_cast<float>(anim.pattern_width),
        static_cast<float>(anim.pattern_height),
        texture_x, texture_y,
        anim.pattern_width,
        anim.pattern_height,
        params
    );
}