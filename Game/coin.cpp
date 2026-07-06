/*============================================================================
Contents   :  [coin.cpp]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#include "coin.h"
#include "sprite.h"
#include "texture.h"
#include "flipbook_animation.h"
#include "config.h"

static constexpr int   COIN_CELL_W = 400;
static constexpr int   COIN_CELL_H = 400;
static constexpr int   COIN_PATTERNS = 20;
static constexpr int   COIN_COLS = 5;
static constexpr float COIN_FPS = 12.0f;
static constexpr float COIN_SCALE = 0.25f;
static constexpr float COIN_SCALE_BIG = 0.50f;
static constexpr float COIN_SCALE_MAX = 1.00f;

static constexpr float COIN_1_WORLD_X = 860.0f;
static constexpr float COIN_2_WORLD_X = 980.0f;
static constexpr float COIN_3_WORLD_X = 1800.0f;
static constexpr float COIN_4_WORLD_X = 2000.0f;
static constexpr float COIN_5_WORLD_X = 3000.0f;

static constexpr float COIN_WORLD_Y = static_cast<float>(SCREEN_HEIGHT) - 450.0f;
static constexpr float COIN_WORLD_Y_MID = static_cast<float>(SCREEN_HEIGHT) - 600.0f;
static constexpr float COIN_WORLD_Y_TOP = static_cast<float>(SCREEN_HEIGHT) - 800.0f;

static constexpr float COIN_COLLECT_R = 100.0f;

static constexpr float RUNNER_SPEED_REF = 220.0f;
static constexpr float GRAVITY_REF = 1200.0f;
static constexpr float GROUND_CY = static_cast<float>(SCREEN_HEIGHT) - 150.0f;
static constexpr float RUNNER_HALF_W = 70.0f;   

static constexpr float JUMP_LEAD = 100.0f;

static float ComputeJumpVelocity(float coin_centre_y, float coin_draw_half_w)
{
    float centre_gap = JUMP_LEAD + coin_draw_half_w - RUNNER_HALF_W;
    float t = centre_gap / RUNNER_SPEED_REF;
    float rise = GROUND_CY - coin_centre_y;   
    return -(rise + 0.5f * GRAVITY_REF * t * t) / t; 
}

struct Coin
{
    float world_x;
    float y;            
    float scale;         
    float jump_velocity; 
    bool  collected;
};

static Coin g_Coins[5];
static int  g_TextureId = -1;
static int  g_AnimId = -1;

void Coin_Initialize()
{
    g_TextureId = Texture_Load(L"assets/textures/coin_anim.png", false);

    g_AnimId = FlipBookAnimation_Create(
        g_TextureId,
        COIN_CELL_W, COIN_CELL_H,
        COIN_PATTERNS, COIN_COLS,
        1.0f / COIN_FPS
    );

    float small_half_w = COIN_CELL_W * COIN_SCALE * 0.5f; 
    float mid_half_w = COIN_CELL_W * COIN_SCALE_BIG * 0.5f; 
    float top_half_w = COIN_CELL_W * COIN_SCALE_MAX * 0.5f;  


    float small_cy = COIN_WORLD_Y + COIN_CELL_H * COIN_SCALE * 0.5f;
    float mid_cy = COIN_WORLD_Y_MID + COIN_CELL_H * COIN_SCALE_BIG * 0.5f;
    float top_cy = COIN_WORLD_Y_TOP + COIN_CELL_H * COIN_SCALE_MAX * 0.5f;

    float small_vel = ComputeJumpVelocity(small_cy, small_half_w);
    float mid_vel = ComputeJumpVelocity(mid_cy, mid_half_w);
    float top_vel = ComputeJumpVelocity(top_cy, top_half_w);

    g_Coins[0] = { COIN_1_WORLD_X, COIN_WORLD_Y,     COIN_SCALE,     small_vel, false };
    g_Coins[1] = { COIN_2_WORLD_X, COIN_WORLD_Y,     COIN_SCALE,     small_vel, false };
    g_Coins[2] = { COIN_3_WORLD_X, COIN_WORLD_Y_MID, COIN_SCALE_BIG, mid_vel,   false };
    g_Coins[3] = { COIN_4_WORLD_X, COIN_WORLD_Y_MID + 100.0f, COIN_SCALE_BIG, mid_vel,   false };
    g_Coins[4] = { COIN_5_WORLD_X, COIN_WORLD_Y_TOP, COIN_SCALE_MAX, top_vel,   false };
}

void Coin_Finalize()
{

}

bool Coin_AllCollected()
{
    for (const Coin& coin : g_Coins)
    {
        if (!coin.collected) return false;
    }
    return true;
}

float Coin_GetJumpVelocityForNearest(float runner_world_x, float lead_distance)
{
    for (const Coin& coin : g_Coins)
    {
        if (coin.collected) continue;

        float dist = coin.world_x - runner_world_x;  
        if (dist > 0.0f && dist <= lead_distance)
        {
            return coin.jump_velocity;
        }
    }
    return 0.0f;
}

void Coin_Update(float runner_world_cx, float runner_world_cy)
{
    for (Coin& coin : g_Coins)
    {
        if (coin.collected) continue;

        float draw_size = COIN_CELL_W * coin.scale;
        float coin_cx = coin.world_x + draw_size * 0.5f;
        float coin_cy = coin.y + draw_size * 0.5f;

        float dx = runner_world_cx - coin_cx;
        float dy = runner_world_cy - coin_cy;

        if ((dx * dx + dy * dy) < (COIN_COLLECT_R * COIN_COLLECT_R))
        {
            coin.collected = true;
        }
    }
}

void Coin_Draw(float camera_x)
{
    for (const Coin& coin : g_Coins)
    {
        if (coin.collected) continue;

        float draw_size = COIN_CELL_W * coin.scale;
        float screen_x = coin.world_x - camera_x;

        if (screen_x + draw_size < 0.0f || screen_x > SCREEN_WIDTH) continue;

        SpriteDrawParams p;
        p.scale = { coin.scale, coin.scale };
        FlipBookAnimation_Draw(g_AnimId, screen_x, coin.y, draw_size, draw_size, p);
    }
}