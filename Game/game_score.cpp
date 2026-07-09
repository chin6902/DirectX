/*============================================================================
Contents   :  [game_score.cpp]

Author     : Chin Qing You
LastUpdate : 2026/07/08
-----------------------------------------------------------------------------

============================================================================*/
#include <DirectXMath.h>

#include "game_score.h"
#include "texture.h"
#include "sprite.h"

using namespace DirectX;

static int g_digit = 0; // Number of digits to display
static int g_score = 0; // Current score
static float g_targetScore = 0.0f;
static float g_accumulatedTime = 0.0f;

static int g_score_texture_id = -1;
static constexpr int NUMBER_CELL_W = 129;
static constexpr int NUMBER_CELL_H = 129;
static constexpr int NUMBER_FRAME_MAX = 15;
static constexpr int NUMBER_COLS = 5;
static constexpr int NUMBER_ROWS = 3;      // 15 frames / 5 cols

void DrawNumber(int number, float x, float y, float scale, XMFLOAT3 color);
void DrawExclamation(float x, float y, float scale, XMFLOAT3 color);

void GameScore_Initialize(int digit)
{
    g_score_texture_id = Texture_Load(L"assets/textures/num.png");

    g_digit = digit;
    g_score = 0;
	g_targetScore = 0.0f;
	g_accumulatedTime = 0.0f;
}

void GameScore_Finalize()
{
    Texture_Release(g_score_texture_id);
}

void GameScore_SetScore(int score)
{
    g_score = static_cast<int>(g_targetScore);
    g_targetScore = static_cast<float>(score);
}

void GameScore_Update(float delta_time)
{
    g_accumulatedTime += delta_time;

    // Simple linear interpolation for score animation
    if (g_accumulatedTime >= 0.1f)
    {
        g_accumulatedTime -= 0.1f;
        g_score += 1;
		if (g_score > static_cast<int>(g_targetScore))
		{
			g_score = static_cast<int>(g_targetScore);
		}
    }
}

void GameScore_Draw(float x, float y, float scale, XMFLOAT3 color)
{
    int value = g_score;

    for (int i = g_digit - 1; i >= 0; --i)
    {
        int divisor = 1;

        for (int d = 0; d < i; ++d)
        {
            divisor *= 10;
        }

        int digit_value = (value / divisor) % 10;

        float digit_x = x + (g_digit - 1 - i) * (NUMBER_CELL_W * scale);
        DrawNumber(digit_value, digit_x, y, scale, color);
    }
}

void DrawNumber(int number, float x, float y, float scale, XMFLOAT3 color)
{
    if (number < 0 || number >= 10)
    {
        return;
    }

    int col = number % NUMBER_COLS;
    int row = number / NUMBER_COLS;

    SpriteDrawParams p;
    p.color = color;

    Sprite_Draw(
        g_score_texture_id,
        x, y,
        NUMBER_CELL_W * scale, NUMBER_CELL_H * scale,
        static_cast<float>(col * NUMBER_CELL_W),
        static_cast<float>(row * NUMBER_CELL_H),
        NUMBER_CELL_W, NUMBER_CELL_H,
        p
    );
}

void DrawExclamation(float x, float y, float scale, XMFLOAT3 color)
{
    constexpr int col = 2;
    constexpr int row = 2;

    SpriteDrawParams p;
    p.color = color;

    Sprite_Draw(
        g_score_texture_id,
        x, y,
        NUMBER_CELL_W * scale, NUMBER_CELL_H * scale,
        static_cast<float>(col * NUMBER_CELL_W),
        static_cast<float>(row * NUMBER_CELL_H),
        NUMBER_CELL_W, NUMBER_CELL_H,
        p
    );
}