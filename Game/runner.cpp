/*============================================================================
Contents   :  [runner.cpp]

Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------

============================================================================*/
#include "runner.h"
#include "sprite.h"
#include "texture.h"
#include "flipbook_animation.h"
#include "config.h"

static constexpr int   RUNMAN_W = 140;
static constexpr int   RUNMAN_H = 200;
static constexpr int   RUNMAN_PATTERNS = 10;
static constexpr int   RUNMAN_COLS = 5;
static constexpr float RUNMAN_FPS = 12.0f;

static constexpr float RUNNER_SPEED = 220.0f;
static constexpr float GRAVITY = 1200.0f;
static constexpr float GROUND_Y = static_cast<float>(SCREEN_HEIGHT) - 250.0f;

static constexpr float HIT_VEL_X = 600.0f;  
static constexpr float HIT_VEL_Y = -900.0f;  
static constexpr float HIT_SPIN = 10.0f;  

enum class RunnerState { RUNNING, JUMPING, HIT, GONE };

static RunnerState g_State = RunnerState::RUNNING;
static float       g_WorldX = 50.0f;
static float       g_Y = GROUND_Y;
static float       g_VelY = 0.0f;
static float       g_VelX = 0.0f;   
static float       g_Angle = 0.0f;  

static int g_TextureId = -1;
static int g_AnimId = -1;

void Runner_Initialize()
{
    g_TextureId = Texture_Load(L"assets/textures/runningman003.png", false);

    g_AnimId = FlipBookAnimation_Create(
        g_TextureId,
        RUNMAN_W, RUNMAN_H,
        RUNMAN_PATTERNS, RUNMAN_COLS,
        1.0f / RUNMAN_FPS
    );

    g_State = RunnerState::RUNNING;
    g_WorldX = 50.0f;
    g_Y = GROUND_Y;
    g_VelY = 0.0f;
    g_VelX = 0.0f;
    g_Angle = 0.0f;
}

void Runner_Finalize() {}

void Runner_TriggerJump(float velocity)
{
    if (g_State != RunnerState::RUNNING) return;

    g_State = RunnerState::JUMPING;
    g_VelY = velocity;
}

void Runner_TriggerHit()
{
    if (g_State == RunnerState::HIT || g_State == RunnerState::GONE) return;

    g_State = RunnerState::HIT;
    g_VelX = HIT_VEL_X;
    g_VelY = HIT_VEL_Y;
    g_Angle = 0.0f;
}

void Runner_Update(float delta_time)
{
    switch (g_State)
    {
    case RunnerState::RUNNING:
        g_WorldX += RUNNER_SPEED * delta_time;
        break;

    case RunnerState::JUMPING:
        g_WorldX += RUNNER_SPEED * delta_time;
        g_VelY += GRAVITY * delta_time;
        g_Y += g_VelY * delta_time;

        if (g_Y >= GROUND_Y)
        {
            g_Y = GROUND_Y;
            g_VelY = 0.0f;
            g_State = RunnerState::RUNNING;
        }
        break;

    case RunnerState::HIT:
        g_WorldX += g_VelX * delta_time;
        g_Y += g_VelY * delta_time;
        g_Angle += HIT_SPIN * delta_time;

        if (g_Y + RUNMAN_H < 0.0f)
        {
            g_State = RunnerState::GONE;
        }
        break;

    case RunnerState::GONE:
        break; 
    }
}

void Runner_Draw(float camera_x)
{
    if (g_State == RunnerState::GONE) return;

    SpriteDrawParams p;
    p.flip_x = true;       
    p.angle = g_Angle;   

    float screen_x = g_WorldX - camera_x;
    FlipBookAnimation_Draw(g_AnimId, screen_x, g_Y, p);
}

float Runner_GetWorldX() { return g_WorldX; }
float Runner_GetY() { return g_Y; }
int   Runner_GetWidth() { return RUNMAN_W; }
int   Runner_GetHeight() { return RUNMAN_H; }

bool Runner_IsOnGround() { return g_State == RunnerState::RUNNING; }
bool Runner_IsHit() { return g_State == RunnerState::HIT; }
bool Runner_IsGone() { return g_State == RunnerState::GONE; }