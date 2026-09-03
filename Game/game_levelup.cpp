/*============================================================================
Contents   :  [game_levelup.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/24
-----------------------------------------------------------------------------

============================================================================*/
#include <cstdio>

#include "game_levelup.h"
#include "game_progress.h"
#include "game_item.h"
#include "game_text.h"
#include "texture.h"
#include "sprite.h"
#include "input_keyboard.h"
#include "input_mouse.h"
#include "game_audio.h"
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

static int   g_LastMouseX = -1;
static int   g_LastMouseY = -1;

static int g_white_texture_id = -1;

void GameLevelUp_Initialize()
{
	g_white_texture_id = Texture_Load(L"assets/textures/white_debug.png", false);
	g_Cursor = 0;
	g_WasOpen = false;
	g_PulseTime = 0.0f;
	g_LastMouseX = -1;
	g_LastMouseY = -1;
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

	if (opt.kind == UPGRADE_ITEM)
	{
		switch (opt.item)
		{
		case ITEM_FIRE_STAFF:     return { 0.95f, 0.35f, 0.12f };
		case ITEM_ELECTRIC_STAFF: return { 1.00f, 0.88f, 0.25f };
		case ITEM_POTION:         return { 0.25f, 0.80f, 0.45f };
		default:                  return { 0.60f, 0.60f, 0.60f };
		}
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

static constexpr float ICON_MAX = 96.0f;
static constexpr float ICON_CENTER_Y = CARD_Y + 285.0f;

static void DrawItemIcon(int texture_id, float center_x, float center_y, float alpha)
{
	if (texture_id < 0) { return; }

	const float tw = static_cast<float>(Texture_GetWidth(texture_id));
	const float th = static_cast<float>(Texture_GetHeight(texture_id));
	if (tw <= 0.0f || th <= 0.0f) { return; }

	const float scale = ICON_MAX / ((tw > th) ? tw : th);
	const float w = tw * scale;
	const float h = th * scale;

	SpriteDrawParams p;
	p.alpha = alpha;

	Sprite_Draw(texture_id,
		center_x - w * 0.5f,
		center_y - h * 0.5f,
		w, h, p);
}

static int CardAtPoint(float mx, float my, int count)
{
	for (int i = 0; i < count; i++)
	{
		const float x = CardX(i, count);
		if (mx >= x && mx <= x + CARD_W && my >= CARD_Y && my <= CARD_Y + CARD_H)
		{
			return i;
		}
	}
	return -1;
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

	const int previous_cursor = g_Cursor;

	// --- keyboard ---
	if (InputKeyboard_IsTrigger(KK_A) || InputKeyboard_IsTrigger(KK_LEFT))
	{
		g_Cursor = (g_Cursor + count - 1) % count;
	}
	if (InputKeyboard_IsTrigger(KK_D) || InputKeyboard_IsTrigger(KK_RIGHT))
	{
		g_Cursor = (g_Cursor + 1) % count;
	}

	// --- mouse ---
	const int mx = InputMouse_GetX();
	const int my = InputMouse_GetY();
	const bool mouse_moved = (mx != g_LastMouseX || my != g_LastMouseY);
	g_LastMouseX = mx;
	g_LastMouseY = my;

	const int hovered = CardAtPoint(static_cast<float>(mx), static_cast<float>(my), count);
	if (mouse_moved && hovered >= 0) { g_Cursor = hovered; }

	if (g_Cursor != previous_cursor) { GameAudio_Play(SND_BUTTON_SELECT); }

	// --- confirm ---
	int chosen = -1;

	if (InputKeyboard_IsTrigger(KK_D1)) { chosen = 0; }
	if (InputKeyboard_IsTrigger(KK_D2)) { chosen = 1; }
	if (InputKeyboard_IsTrigger(KK_D3)) { chosen = 2; }

	if (InputKeyboard_IsTrigger(KK_ENTER)) { chosen = g_Cursor; }

	if (InputMouse_IsTrigger(MOUSE_BUTTON_LEFT) && hovered >= 0) { chosen = hovered; }

	if (chosen >= 0 && chosen < count)
	{
		GameAudio_Play(SND_BUTTON_CHOOSE);
		GameProgress_ChooseOffer(chosen);
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

		// pulse outline
		if (selected)
		{
			const float pulse = 0.5f + 0.5f * sinf(g_PulseTime * 6.0f);
			const float pad = 6.0f + pulse * 4.0f;
			DrawRect(x - pad, CARD_Y - pad, CARD_W + pad * 2, CARD_H + pad * 2,
				col, 0.55f + pulse * 0.35f);
		}

		DrawRect(x, CARD_Y, CARD_W, CARD_H, { 0.10f, 0.11f, 0.14f }, 0.95f);

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
			if (col_i >= 15 && *p == ' ')         
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

		if (opt.kind == UPGRADE_ITEM)
		{
			DrawItemIcon(GameItem_GetItemTexture(opt.item),
				x + CARD_W * 0.5f,
				ICON_CENTER_Y,
				selected ? 1.0f : 0.85f);
		}

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
		"A / D or MOUSE  SELECT      ENTER / CLICK  CONFIRM", 0.45f,
		{ 0.6f, 0.62f, 0.7f });

	Sprite_SetFilter(kSpriteFilter_Point);
}