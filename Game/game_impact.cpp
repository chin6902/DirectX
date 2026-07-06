/*============================================================================
Contents   :  [game_impact.cpp]

Author     : Chin Qing You
LastUpdate : 2026/07/06
-----------------------------------------------------------------------------

============================================================================*/
#include "texture.h"
#include "game_impact.h"
#include "flipbook_animation.h"

// ============================================================================
// ExplosionInfo — static, per-TYPE definition.
// One entry per ExplosionType, shared by every instance of that type.
// Filled in once by Create(), never changes at runtime.
// ============================================================================
struct ExplosionInfo
{
    const wchar_t* texture_filename;
    int   texture_id;      // filled in during Create()
    int   cell_width;
    int   cell_height;
    int   frame_count;
    int   column_count;
    float frame_time;
    int   frame_start;
    int   frame_end;
    float scale;
};

static constexpr int ExplosionType_Count = 2;

// TODO: confirm real cell_width/cell_height against the actual texture file —
// 427x240 doesn't divide evenly into a 4x2 grid, so these are placeholders.
static ExplosionInfo g_ExplosionInfo[ExplosionType_Count] =
{
    // ExplosionType_Large (explosion.png) — 4 cols x 2 rows, 8 frames
    { L"assets/textures/explosion.png", -1, 106, 120, 8, 4, 0.08f, 0, 7 , 1.0f },
    { L"assets/textures/explosion.png", -1, 106, 120, 8, 4, 0.08f, 0, 7 , 0.5f },
};

// ============================================================================
// ExplosionData — per-INSTANCE runtime state.
// One entry per active/inactive slot. Tracks where a specific playing
// instance is and which FlipBookAnimation it owns.
// anim_id is set once by Create(); is_active/x/y are reset by Initialize().
// ============================================================================
struct ExplosionData
{
    int   anim_id;
    float x;
    float y;
    bool  is_active;
};

static constexpr int IMPACT_MAX_PER_TYPE{ 32 };
static ExplosionData g_ExplosionData[ExplosionType_Count][IMPACT_MAX_PER_TYPE]{};

// ============================================================================
// Create — sets up resources: loads each type's texture and creates one
// FlipBookAnimation instance per slot. Call this once, at program startup.
// ============================================================================
void Game_Impact_Create()
{
    for (int type = 0; type < ExplosionType_Count; type++)
    {
        ExplosionInfo& info = g_ExplosionInfo[type];
        info.texture_id = Texture_Load(info.texture_filename);

        for (ExplosionData& data : g_ExplosionData[type])
        {
            data.anim_id = FlipBookAnimation_Create(
                info.texture_id,
                info.cell_width,
                info.cell_height,
                info.frame_count,
                info.column_count,
                info.frame_time
            );

            FlipBookAnimation_SetRange(data.anim_id, info.frame_start, info.frame_end);
            FlipBookAnimation_SetMode(data.anim_id, AnimPlayMode::ONE_SHOT);
        }
    }
}

// ============================================================================
// Initialize — resets per-instance runtime state only (is_active, position).
// Does NOT touch anim_id, textures, or FlipBookAnimation instances — those
// come from Create(). Safe to call again on a level/game restart.
// ============================================================================
void Game_Impact_Initialize()
{
    for (int type = 0; type < ExplosionType_Count; type++)
    {
        for (ExplosionData& data : g_ExplosionData[type])
        {
            data.x = 0.0f;
            data.y = 0.0f;
            data.is_active = false;
        }
    }
}

void Game_Impact_Finalize()
{
    for (int type = 0; type < ExplosionType_Count; type++)
    {
        for (ExplosionData& data : g_ExplosionData[type])
        {
            FlipBookAnimation_Destroy(data.anim_id);
            data.anim_id = -1;
            data.is_active = false;
        }

        Texture_Release(g_ExplosionInfo[type].texture_id);
        g_ExplosionInfo[type].texture_id = -1;
    }
}

void Game_Impact_Trigger(ExplosionType type, float x, float y)
{
    const ExplosionInfo& info = g_ExplosionInfo[type];

    for (ExplosionData& data : g_ExplosionData[type])
    {
        if (data.is_active)
        {
            continue;
        }

        FlipBookAnimation_SetFrame(data.anim_id, info.frame_start);
        FlipBookAnimation_SetMode(data.anim_id, AnimPlayMode::ONE_SHOT);

        data.x = x;
        data.y = y;
        data.is_active = true;
        return;
    }

    // All slots of this type busy — impact silently dropped.
}

void Game_Impact_Update(float delta_time)
{
    for (int type = 0; type < ExplosionType_Count; type++)
    {
        for (ExplosionData& data : g_ExplosionData[type])
        {
            if (!data.is_active)
            {
                continue;
            }

            if (FlipBookAnimation_IsFinished(data.anim_id))
            {
                data.is_active = false;
            }
        }
    }
}

void Game_Impact_Draw()
{
    for (int type = 0; type < ExplosionType_Count; type++)
    {
        const ExplosionInfo& info = g_ExplosionInfo[type];
        float draw_w = info.cell_width * info.scale;
        float draw_h = info.cell_height * info.scale;

        for (const ExplosionData& data : g_ExplosionData[type])
        {
            if (!data.is_active)
            {
                continue;
            }

            FlipBookAnimation_Draw(data.anim_id, data.x - draw_w * 0.5f, data.y - draw_h * 0.5f, draw_w, draw_h);
        }
    }
}