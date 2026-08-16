/*============================================================================
Contents   :  [game_impact.cpp]

Author     : Chin Qing You
LastUpdate : 2026/07/06
-----------------------------------------------------------------------------

============================================================================*/
#include "texture.h"
#include "game_impact.h"
#include "flipbook_animation.h"
#include "Audio.h"

struct ExplosionInfo
{
    const wchar_t* texture_filename;
    int   texture_id;      
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

static ExplosionInfo g_ExplosionInfo[ExplosionType_Count] =
{
	// ExplosionType_Large
    { L"assets/textures/explosion.png", -1, 106, 120, 8, 4, 0.08f, 0, 7 , 1.0f },

	// ExplosionType_Small
    { L"assets/textures/explosion.png", -1, 106, 120, 8, 4, 0.08f, 0, 7 , 0.5f },
};

struct ExplosionData
{
    int   anim_id;
    float x;
    float y;
    bool  is_active;
};

static int g_explosionSoundID = -1;

static constexpr int IMPACT_MAX_PER_TYPE{ 32 };
static ExplosionData g_ExplosionData[ExplosionType_Count][IMPACT_MAX_PER_TYPE]{};

void GameImpact_Create()
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

void GameImpact_Initialize()
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

    g_explosionSoundID = LoadAudio("assets/sounds/explosion.wav");
}

void GameImpact_Finalize()
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

	UnloadAudio(g_explosionSoundID);
}

void GameImpact_Trigger(ExplosionType type, float x, float y)
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
        //PlayAudio(g_explosionSoundID);
        return;
    }
}

void GameImpact_Update(float delta_time)
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

void GameImpact_Draw()
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