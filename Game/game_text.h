/*============================================================================
Contents   :  [game_text.h]

Author     : Chin Qing You
LastUpdate : 2026/08/14
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_TEXT_H
#define GAME_TEXT_H

#include <DirectXMath.h>

void GameText_Initialize();
void GameText_Finalize();

void GameText_Draw(float x, float y, const char* text, float scale,
    const DirectX::XMFLOAT3& color = { 1.0f, 1.0f, 1.0f },
    float alpha = 1.0f);

void GameText_DrawCentered(float center_x, float y, const char* text, float scale,
    const DirectX::XMFLOAT3& color = { 1.0f, 1.0f, 1.0f },
    float alpha = 1.0f);

float GameText_Measure(const char* text, float scale);

#endif