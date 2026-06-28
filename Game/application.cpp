/*============================================================================
Contents   :  [application.cpp]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#include "application.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"
#include "config.h"
#include "sprite.h"
#include "flipbook_animation.h"
#include "runner.h"
#include "coin.h"
#include "food.h"

static int g_TextureId_Bg = TEXTURE_INVALID_ID;

static constexpr float RUNNER_SCREEN_X = 250.0f;
static float g_CameraX = 0.0f;
static bool  g_CameraFrozen = false;

static bool g_FoodSpawned = false;

static void DrawBackground();

bool Application_Initialize(HWND hWnd)
{
    if (!Direct3DInitialize(hWnd)) return false;

    Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
    Sprite_Initialize();
    FlipBookAnimation_Initialize();

    g_TextureId_Bg = Texture_Load(L"assets/textures/Background.png", false);

    Runner_Initialize();
    Coin_Initialize();
    Food_Initialize();

    return true;
}

void Application_Finalize()
{
    Food_Finalize();
    Coin_Finalize();
    Runner_Finalize();

    FlipBookAnimation_Finalize();
    Sprite_Finalize();
    Texture_Finalize();
    Shader_Finalize();
    Direct3DFinalize();
}

void Application_Update(float delta_time)
{
    Runner_Update(delta_time);


    if (!g_CameraFrozen)
    {
        float world_x = Runner_GetWorldX();
        g_CameraX = (world_x > RUNNER_SCREEN_X) ? world_x - RUNNER_SCREEN_X : 0.0f;
    }

    if (!Runner_IsHit() && !Runner_IsGone())
    {
        float world_x = Runner_GetWorldX();

        float jump_vel = Coin_GetJumpVelocityForNearest(world_x, 150.0f);
        if (jump_vel != 0.0f)
        {
            Runner_TriggerJump(jump_vel);
        }

        float runner_world_cx = world_x + Runner_GetWidth() * 0.5f;
        float runner_world_cy = Runner_GetY() + Runner_GetHeight() * 0.5f;
        Coin_Update(runner_world_cx, runner_world_cy);

        if (!g_FoodSpawned && Coin_AllCollected() && Runner_IsOnGround())
        {
            Food_Spawn(Runner_GetY());
            g_FoodSpawned = true;
        }
    }

    if (g_FoodSpawned && !Food_HasHit())
    {
        Food_Update(delta_time);

        float runner_screen_x = Runner_GetWorldX() - g_CameraX;
        float runner_cx = runner_screen_x + Runner_GetWidth() * 0.5f;
        float runner_cy = Runner_GetY() + Runner_GetHeight() * 0.5f;

        if (Food_CheckAndApplyHit(runner_cx, runner_cy))
        {
            Runner_TriggerHit();    
            g_CameraFrozen = true;  
        }
    }

    FlipBookAnimation_Update(delta_time);
}

void Application_FixedUpdate()
{

}

void Application_Draw()
{
    Sprite_SetFilter(kSpriteFilter_Linear);

    DrawBackground();          
    Coin_Draw(g_CameraX);       
    Food_Draw();              
    Runner_Draw(g_CameraX);    
}

static void DrawBackground()
{
    Sprite_Draw(
        g_TextureId_Bg,
        0.0f, 0.0f,
        static_cast<float>(SCREEN_WIDTH),
        static_cast<float>(SCREEN_HEIGHT),
        g_CameraX, 0.0f,
        Texture_GetWidth(g_TextureId_Bg),
        Texture_GetHeight(g_TextureId_Bg)
    );
}