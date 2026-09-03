/*============================================================================
Contents   :  [game_ui.h]

Author     : Chin Qing You
LastUpdate : 2026/08/15
-----------------------------------------------------------------------------

============================================================================*/
#ifndef GAME_UI_H
#define GAME_UI_H

#include <DirectXMath.h>

void GameUI_Initialize();
void GameUI_Finalize();
void GameUI_Update(float delta_time);
void GameUI_Draw();

void GameUI_Flash(const DirectX::XMFLOAT3& color, float duration);

void GameUI_DrawScreenRect(float x, float y, float w, float h, const DirectX::XMFLOAT3& color, float alpha = 1.0f);

#endif