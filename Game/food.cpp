/*============================================================================
Contents   :  [food.cpp]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#include "food.h"
#include "sprite.h"
#include "texture.h"
#include "config.h"

static constexpr float FOOD_SPEED = 1000.0f;

static constexpr float FOOD_HIT_R = 90.0f;

static constexpr float FOOD_DRAW_W = 128.0f;
static constexpr float FOOD_DRAW_H = 128.0f;

static int   g_TextureId = -1;

static float g_ScreenX = 0.0f;
static float g_ScreenY = 0.0f;
static bool  g_Spawned = false;
static bool  g_Hit = false;

void Food_Initialize()
{
    g_TextureId = Texture_Load(L"assets/textures/food.png", false);
    g_Spawned = false;
    g_Hit = false;
}

void Food_Finalize()
{
}

void Food_Spawn(float screen_y)
{
    g_ScreenX = static_cast<float>(SCREEN_WIDTH);
    g_ScreenY = screen_y;
    g_Spawned = true;
    g_Hit = false;
}

void Food_Update(float delta_time)
{
    if (!g_Spawned || g_Hit) return;

    g_ScreenX -= FOOD_SPEED * delta_time;
}

bool Food_CheckAndApplyHit(float runner_screen_cx, float runner_screen_cy)
{
    if (!g_Spawned || g_Hit) return false;

    float food_cx = g_ScreenX + FOOD_DRAW_W * 0.5f;
    float food_cy = g_ScreenY + FOOD_DRAW_H * 0.5f;

    float dx = runner_screen_cx - food_cx;
    float dy = runner_screen_cy - food_cy;

    if ((dx * dx + dy * dy) < (FOOD_HIT_R * FOOD_HIT_R))
    {
        g_Hit = true;  
        return true;
    }
    return false;
}

void Food_Draw()
{
    if (!g_Spawned) return;

    Sprite_SetFilter(kSpriteFilter_Linear);
    Sprite_Draw(g_TextureId, g_ScreenX, g_ScreenY, FOOD_DRAW_W, FOOD_DRAW_H);
}

bool Food_IsSpawned() { return g_Spawned; }
bool Food_HasHit() { return g_Hit; }