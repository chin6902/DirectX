/*============================================================================
Contents   :  [game_text.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/14
-----------------------------------------------------------------------------

============================================================================*/
#include "game_text.h"
#include "texture.h"
#include "sprite.h"

using namespace DirectX;

// --- sheet layout ---
static constexpr int   FONT_CELL = 32;    
static constexpr int   FONT_COLS = 16;
static constexpr int   FONT_FIRST = 32;    
static constexpr int   FONT_LAST = 126;   

static constexpr float ADVANCE_RATIO = 0.62f;

static int g_font_texture_id = -1;

void GameText_Initialize()
{
	g_font_texture_id = Texture_Load(L"assets/textures/Sixtyfour-Regular_ascii_512.png");
}

void GameText_Finalize()
{
	Texture_Release(g_font_texture_id);
}

float GameText_Measure(const char* text, float scale)
{
	if (text == nullptr) { return 0.0f; }

	int n = 0;
	for (const char* p = text; *p != '\0'; ++p) { n++; }

	return n * (FONT_CELL * ADVANCE_RATIO * scale);
}

void GameText_Draw(float x, float y, const char* text, float scale,
	const XMFLOAT3& color, float alpha)
{
	if (text == nullptr || g_font_texture_id == TEXTURE_INVALID_ID) { return; }

	const float advance = FONT_CELL * ADVANCE_RATIO * scale;
	const float size = FONT_CELL * scale;

	SpriteDrawParams p;
	p.color = color;
	p.alpha = alpha;

	float pen_x = x;
	for (const char* c = text; *c != '\0'; ++c)
	{
		const unsigned char ch = static_cast<unsigned char>(*c);

		// space still advances the pen, it just draws nothing
		if (ch >= FONT_FIRST && ch <= FONT_LAST && ch != ' ')
		{
			const int index = ch - FONT_FIRST;
			const int col = index % FONT_COLS;
			const int row = index / FONT_COLS;

			Sprite_Draw(
				g_font_texture_id,
				pen_x, y,
				size, size,
				static_cast<float>(col * FONT_CELL),
				static_cast<float>(row * FONT_CELL),
				FONT_CELL, FONT_CELL,
				p);
		}

		pen_x += advance;
	}
}

void GameText_DrawCentered(float center_x, float y, const char* text, float scale,
	const XMFLOAT3& color, float alpha)
{
	GameText_Draw(center_x - GameText_Measure(text, scale) * 0.5f, y, text, scale,
		color, alpha);
}