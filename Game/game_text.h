/*============================================================================
Contents   :  [game_text.h]

Author     : Chin Qing You
LastUpdate : 2026/08/14
-----------------------------------------------------------------------------
Bitmap text. Generalises what game_score does with digits to the whole
printable ASCII range, so menus and labels can be drawn.

Sheet layout (Sixtyfour-Regular_ascii_512.png):
  512x512, a 16x16 grid of 32px cells.
  Cell index = character - 32, so index 0 is space and index 94 is '~'.
  Glyphs are white on transparent, so SpriteDrawParams.color tints them.

All coordinates are SCREEN space: this is UI, it does not go through
the camera.
============================================================================*/
#ifndef GAME_TEXT_H
#define GAME_TEXT_H

#include <DirectXMath.h>

void GameText_Initialize();
void GameText_Finalize();

// Draws `text` with its top-left corner at (x, y).
// scale 1.0 = 32px glyphs. Unknown characters are skipped.
void GameText_Draw(float x, float y, const char* text, float scale,
    const DirectX::XMFLOAT3& color = { 1.0f, 1.0f, 1.0f });

// Same, but centred horizontally on `center_x`.
void GameText_DrawCentered(float center_x, float y, const char* text, float scale,
    const DirectX::XMFLOAT3& color = { 1.0f, 1.0f, 1.0f });

// Width the string will occupy, for laying out panels around it.
float GameText_Measure(const char* text, float scale);

#endif