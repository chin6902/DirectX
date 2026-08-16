/*============================================================================
Contents   :  [game_levelup.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/14
-----------------------------------------------------------------------------

============================================================================*/
#include <cstdio>

#include "game_levelup.h"
#include "game_progress.h"
#include "game_text.h"
#include "texture.h"
#include "sprite.h"
#include "input_keyboard.h"
#include "config.h"

using namespace DirectX;

// --- layout ---
static constexpr float CARD_W = 300.0f;
static constexpr float CARD_H = 380.0f;
static constexpr float CARD_GAP = 40.0f;
static constexpr float CARD_Y = 260.0f;

static int   g_Cursor = 0;
static bool  g_WasOpen = false;
static float g_PulseTime = 0.0f;

static int g_white_texture_id = -1;

void GameLevelUp_Initialize()
{
	g_white_texture_id = Texture_Load(L"assets/textures/white_debug.png", false);
	g_Cursor = 0;
	g_WasOpen = false;
	g_PulseTime = 0.0f;
}

void GameLevelUp_Finalize()
{
	Texture_Release(g_white_texture_id);
}

static XMFLOAT3 OfferColor(const UpgradeOption& opt)
{
	if (opt.kind == UPGRADE_STAT)
	{
		return { 0.55f, 0.58f, 0.65f };      
	}

	switch (opt.element)
	{
	case ELEMENT_FIRE:    return { 0.95f, 0.35f, 0.12f };
	case ELEMENT_ICE:     return { 0.30f, 0.70f, 1.00f };
	case ELEMENT_THUNDER: return { 1.00f, 0.88f, 0.25f };
	default:              return { 0.60f, 0.60f, 0.60f };
	}
}

static void DrawRect(float x, float y, float w, float h, const XMFLOAT3& color, float alpha)
{
	SpriteDrawParams p;
	p.color = color;
	p.alpha = alpha;
	Sprite_Draw(g_white_texture_id, x, y, w, h, p);
}

static float CardX(int index, int count)
{
	const float total = count * CARD_W + (count - 1) * CARD_GAP;
	const float left = (SCREEN_WIDTH - total) * 0.5f;
	return left + index * (CARD_W + CARD_GAP);
}

void GameLevelUp_Update(float delta_time)
{
	const int count = GameProgress_GetOfferCount();
	if (count <= 0) { return; }

	if (!g_WasOpen)
	{
		g_Cursor = 0;
		g_WasOpen = true;
	}
	g_PulseTime += delta_time;

	if (InputKeyboard_IsTrigger(KK_A)) { g_Cursor = (g_Cursor + count - 1) % count; }
	if (InputKeyboard_IsTrigger(KK_D)) { g_Cursor = (g_Cursor + 1) % count; }

	int direct = -1;
	if (InputKeyboard_IsTrigger(KK_D1)) { direct = 0; }
	if (InputKeyboard_IsTrigger(KK_D2)) { direct = 1; }
	if (InputKeyboard_IsTrigger(KK_D3)) { direct = 2; }

	const bool confirm = InputKeyboard_IsTrigger(KK_SPACE) || InputKeyboard_IsTrigger(KK_ENTER);

	if (direct >= 0 && direct < count)
	{
		GameProgress_ChooseOffer(direct);
		g_WasOpen = false;
	}
	else if (confirm)
	{
		GameProgress_ChooseOffer(g_Cursor);
		g_WasOpen = false;
	}
}

// ============================================================================
// Draw
// ============================================================================
void GameLevelUp_Draw()
{
	const int count = GameProgress_GetOfferCount();
	if (count <= 0) { return; }

	Sprite_SetFilter(kSpriteFilter_Linear);

	// --- dim ---
	DrawRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT,
		{ 0.0f, 0.0f, 0.0f }, 0.72f);

	// --- heading ---
	char heading[64];
	snprintf(heading, sizeof(heading), "LEVEL %d", GameProgress_GetLevel());
	GameText_DrawCentered(SCREEN_WIDTH * 0.5f, 120.0f, heading, 1.2f,
		{ 1.0f, 1.0f, 1.0f });
	GameText_DrawCentered(SCREEN_WIDTH * 0.5f, 185.0f, "CHOOSE AN UPGRADE", 0.6f,
		{ 0.75f, 0.78f, 0.85f });

	// --- cards ---
	for (int i = 0; i < count; i++)
	{
		const UpgradeOption& opt = GameProgress_GetOffer(i);
		const XMFLOAT3 col = OfferColor(opt);
		const bool selected = (i == g_Cursor);
		const float x = CardX(i, count);

		// selected card gets a pulse outline
		if (selected)
		{
			const float pulse = 0.5f + 0.5f * sinf(g_PulseTime * 6.0f);
			const float pad = 6.0f + pulse * 4.0f;
			DrawRect(x - pad, CARD_Y - pad, CARD_W + pad * 2, CARD_H + pad * 2,
				col, 0.55f + pulse * 0.35f);
		}

		DrawRect(x, CARD_Y, CARD_W, CARD_H, { 0.10f, 0.11f, 0.14f }, 0.95f);

		// colour band across the top: the card's identity at a glance
		DrawRect(x, CARD_Y, CARD_W, 70.0f, col, selected ? 1.0f : 0.75f);

		// number key hint
		char key[8];
		snprintf(key, sizeof(key), "%d", i + 1);
		GameText_DrawCentered(x + CARD_W * 0.5f, CARD_Y + 14.0f, key, 1.0f,
			{ 0.05f, 0.05f, 0.07f });

		// label
		const char* label = opt.label;
		char line[3][32]{};
		int  lines = 0;
		int  col_i = 0;
		for (const char* p = label; *p != '\0' && lines < 3; ++p)
		{
			if (col_i >= 15 && *p == ' ')          // wrap at the next space
			{
				line[lines][col_i] = '\0';
				lines++;
				col_i = 0;
				continue;
			}
			if (col_i < 31) { line[lines][col_i++] = *p; }
		}
		line[lines][col_i] = '\0';
		lines++;

		for (int L = 0; L < lines; L++)
		{
			GameText_DrawCentered(x + CARD_W * 0.5f,
				CARD_Y + 130.0f + L * 44.0f,
				line[L], 0.55f,
				selected ? XMFLOAT3{ 1.0f, 1.0f, 1.0f }
			: XMFLOAT3{ 0.80f, 0.82f, 0.88f });
		}

		// element cards show the level they would move to
		if (opt.kind == UPGRADE_ELEMENT)
		{
			char lv[32];
			snprintf(lv, sizeof(lv), "Lv %d -> %d",
				GameProgress_GetElementLevel(opt.element),
				GameProgress_GetElementLevel(opt.element) + 1);
			GameText_DrawCentered(x + CARD_W * 0.5f, CARD_Y + CARD_H - 70.0f,
				lv, 0.5f, col);
		}
	}

	GameText_DrawCentered(SCREEN_WIDTH * 0.5f, CARD_Y + CARD_H + 50.0f,
		"A / D  SELECT      SPACE/ENTER  CONFIRM", 0.45f,
		{ 0.6f, 0.62f, 0.7f });

	Sprite_SetFilter(kSpriteFilter_Point);
}