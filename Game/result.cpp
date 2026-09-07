/*============================================================================
Contents   :  [result.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/26
-----------------------------------------------------------------------------

============================================================================*/
#include <cstdio>

#include "result.h"
#include "texture.h"
#include "sprite.h"
#include "config.h"
#include "input_keyboard.h"
#include "input_mouse.h"
#include "game_audio.h"
#include "scene.h"
#include "fade.h"
#include "game_text.h"

using namespace DirectX;

static constexpr float BTN_W = 300.0f;
static constexpr float BTN_H = 90.0f;
static constexpr float BTN_GAP = 60.0f;
static constexpr float BTN_Y = 560.0f;

enum ResultChoice
{
	CHOICE_RETRY,
	CHOICE_TITLE,
	CHOICE_COUNT,
};

static constexpr const char* g_ChoiceLabel[CHOICE_COUNT] =
{
	"RETRY",
	"TITLE",
};

// --- carried across the scene boundary; NOT reset in Initialize ---
static bool  g_Cleared = false;
static int   g_LevelReached = 1;
static float g_RunTime = 0.0f;

// --- per-visit state; reset every time ---
static int   g_Cursor = 0;
static float g_PulseTime = 0.0f;
static bool  g_IsChangeScene = false;
static int   g_white_texture_id = -1;
static int   g_LastMouseX = -1;
static int   g_LastMouseY = -1;

void Result_SetOutcome(bool cleared, int level_reached, float run_time)
{
	g_Cleared = cleared;
	g_LevelReached = level_reached;
	g_RunTime = run_time;
}

void Result_Initialize()
{
	g_white_texture_id = Texture_Load(L"assets/textures/white_debug.png", false);
	GameText_Initialize();

	g_Cursor = 0;
	g_PulseTime = 0.0f;
	g_IsChangeScene = false;
	g_LastMouseX = -1;
	g_LastMouseY = -1;
	
	Fade_Start(FADE_IN, 1.0f, { 0.0f, 0.0f, 0.0f, 0.0f });
}

void Result_Finalize()
{
	GameText_Finalize();
	Texture_Release(g_white_texture_id);
}

static float ButtonX(int index)
{
	const float total = CHOICE_COUNT * BTN_W + (CHOICE_COUNT - 1) * BTN_GAP;
	const float left = (SCREEN_WIDTH - total) * 0.5f;
	return left + index * (BTN_W + BTN_GAP);
}

static int ButtonAtPoint(float mx, float my)
{
	for (int i = 0; i < CHOICE_COUNT; i++)
	{
		const float x = ButtonX(i);
		if (mx >= x && mx <= x + BTN_W && my >= BTN_Y && my <= BTN_Y + BTN_H)
		{
			return i;
		}
	}
	return -1;
}

void Result_Update(float delta_time)
{
	g_PulseTime += delta_time;

	if (g_IsChangeScene)
	{
		if (Fade_IsFinished())
		{
			Scene_SetNextScene((g_Cursor == CHOICE_RETRY) ? SCENE_GAME : SCENE_TITLE);
		}
		return;
	}

	const int previous_cursor = g_Cursor;

	if (InputKeyboard_IsTrigger(KK_A) || InputKeyboard_IsTrigger(KK_LEFT))
	{
		g_Cursor = (g_Cursor + CHOICE_COUNT - 1) % CHOICE_COUNT;
	}
	if (InputKeyboard_IsTrigger(KK_D) || InputKeyboard_IsTrigger(KK_RIGHT))
	{
		g_Cursor = (g_Cursor + 1) % CHOICE_COUNT;
	}

	const int mx = InputMouse_GetX();
	const int my = InputMouse_GetY();
	const bool mouse_moved = (mx != g_LastMouseX || my != g_LastMouseY);
	g_LastMouseX = mx;
	g_LastMouseY = my;

	const int hovered = ButtonAtPoint(static_cast<float>(mx), static_cast<float>(my));
	if (mouse_moved && hovered >= 0) { g_Cursor = hovered; }

	if (g_Cursor != previous_cursor) { GameAudio_Play(SND_BUTTON_SELECT); }

	const bool confirm = InputKeyboard_IsTrigger(KK_ENTER) || (InputMouse_IsTrigger(MOUSE_BUTTON_LEFT) && hovered >= 0);

	if (confirm)
	{
		if (hovered >= 0 && InputMouse_IsTrigger(MOUSE_BUTTON_LEFT)) { g_Cursor = hovered; }
		GameAudio_Play(SND_BUTTON_CHOOSE);
		Fade_Start(FADE_OUT, 1.0f, { 0.0f, 0.0f, 0.0f, 1.0f });
		g_IsChangeScene = true;
	}
}

static void DrawRect(float x, float y, float w, float h,
	const XMFLOAT3& color, float alpha)
{
	if (w <= 0.0f || h <= 0.0f) { return; }
	SpriteDrawParams p;
	p.color = color;
	p.alpha = alpha;
	Sprite_Draw(g_white_texture_id, x, y, w, h, p);
}

void Result_Draw()
{
	Sprite_SetFilter(kSpriteFilter_Linear);

	DrawRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT,
		{ 0.04f, 0.04f, 0.06f }, 1.0f);

	const float cx = SCREEN_WIDTH * 0.5f;

	// --- headline ---
	const XMFLOAT3 head_col = g_Cleared ? XMFLOAT3{ 1.00f, 0.90f, 0.45f }
	: XMFLOAT3{ 0.90f, 0.30f, 0.30f };
	GameText_DrawCentered(cx, 170.0f,
		g_Cleared ? "ALL WAVES CLEARED" : "GAME OVER", 1.2f, head_col);

	// --- run summary ---
	char line[64];

	snprintf(line, sizeof(line), "LEVEL REACHED   %d", g_LevelReached);
	GameText_DrawCentered(cx, 300.0f, line, 0.55f, { 0.85f, 0.87f, 0.92f });

	const int minutes = static_cast<int>(g_RunTime) / 60;
	const int seconds = static_cast<int>(g_RunTime) % 60;
	snprintf(line, sizeof(line), "TIME SURVIVED   %d:%02d", minutes, seconds);
	GameText_DrawCentered(cx, 360.0f, line, 0.55f, { 0.85f, 0.87f, 0.92f });

	// --- choices ---
	for (int i = 0; i < CHOICE_COUNT; i++)
	{
		const float x = ButtonX(i);
		const bool  selected = (i == g_Cursor);

		if (selected)
		{
			const float pulse = 0.5f + 0.5f * sinf(g_PulseTime * 6.0f);
			const float pad = 5.0f + pulse * 4.0f;
			DrawRect(x - pad, BTN_Y - pad, BTN_W + pad * 2, BTN_H + pad * 2,
				{ 0.45f, 0.85f, 1.00f }, 0.55f + pulse * 0.35f);
		}

		DrawRect(x, BTN_Y, BTN_W, BTN_H, { 0.10f, 0.11f, 0.14f }, 0.95f);

		GameText_DrawCentered(x + BTN_W * 0.5f, BTN_Y + 26.0f,
			g_ChoiceLabel[i], 0.70f,
			selected ? XMFLOAT3{ 1.0f, 1.0f, 1.0f }
		: XMFLOAT3{ 0.62f, 0.65f, 0.72f });
	}

	GameText_DrawCentered(cx, BTN_Y + BTN_H + 60.0f,
		"A / D or MOUSE  SELECT      ENTER / CLICK  CONFIRM", 0.42f,
		{ 0.55f, 0.58f, 0.66f });

	Sprite_SetFilter(kSpriteFilter_Point);
	Sprite_Flush();
}