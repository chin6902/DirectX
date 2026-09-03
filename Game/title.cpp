/*============================================================================
Contents   :  [title.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/28
-----------------------------------------------------------------------------

============================================================================*/
#include <cmath>

#include "title.h"
#include "texture.h"
#include "sprite.h"
#include "config.h"
#include "scene.h"
#include "fade.h"
#include "game_text.h"
#include "game_audio.h"

#include "input_keyboard.h"
#include "input_mouse.h"

using namespace DirectX;

static constexpr float BTN_W = 300.0f;
static constexpr float BTN_H = 90.0f;
static constexpr float BTN_X = SCREEN_WIDTH * 0.5f - BTN_W * 0.5f;
static constexpr float BTN_Y = SCREEN_HEIGHT * 0.5f + 200.0f;

static int   g_TitleBG_texture = -1;
static int   g_white_texture_id = -1;

static float g_PulseTime = 0.0f;
static bool  g_WasHovered = false;
static bool  g_IsChangeScene = false;

static bool IsButtonHovered()
{
	const float mx = static_cast<float>(InputMouse_GetX());
	const float my = static_cast<float>(InputMouse_GetY());
	return mx >= BTN_X && mx <= BTN_X + BTN_W
		&& my >= BTN_Y && my <= BTN_Y + BTN_H;
}

void Title_Initialize()
{
	g_TitleBG_texture = Texture_Load(L"assets/textures/title1.png");
	g_white_texture_id = Texture_Load(L"assets/textures/white_debug.png", false);
	GameText_Initialize();

	g_PulseTime = 0.0f;
	g_WasHovered = false;
	g_IsChangeScene = false;

	Fade_Start(FADE_IN, 1.0f, { 0.0f, 0.0f, 0.0f, 0.0f });
}

void Title_Finalize()
{
	GameText_Finalize();
	Texture_Release(g_white_texture_id);
	Texture_Release(g_TitleBG_texture);
}

void Title_Update(float delta_time)
{
	g_PulseTime += delta_time;

	if (g_IsChangeScene)
	{
		if (Fade_IsFinished()) { Scene_SetNextScene(SCENE_GAME); }
		return;
	}

	const bool hovered = IsButtonHovered();
	if (hovered && !g_WasHovered) { GameAudio_Play(SND_BUTTON_SELECT); }
	g_WasHovered = hovered;

	const bool confirm = InputKeyboard_IsTrigger(KK_ENTER)
		|| (InputMouse_IsTrigger(MOUSE_BUTTON_LEFT) && hovered);

	if (confirm)
	{
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

void Title_Draw()
{
	Sprite_Draw(g_TitleBG_texture, 0.0f, 0.0f,
		static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT));

	Sprite_SetFilter(kSpriteFilter_Linear);

	const float pulse = 0.5f + 0.5f * sinf(g_PulseTime * 6.0f);
	const float pad = 5.0f + pulse * 4.0f;
	const float glow = g_WasHovered ? 1.0f : 0.75f;

	DrawRect(BTN_X - pad, BTN_Y - pad, BTN_W + pad * 2, BTN_H + pad * 2,
		{ 0.45f, 0.85f, 1.00f }, (0.55f + pulse * 0.35f) * glow);

	DrawRect(BTN_X, BTN_Y, BTN_W, BTN_H, { 0.10f, 0.11f, 0.14f }, 0.95f);

	GameText_DrawCentered(BTN_X + BTN_W * 0.5f, BTN_Y + 26.0f,
		"START", 0.70f, { 1.0f, 1.0f, 1.0f });

	GameText_DrawCentered(SCREEN_WIDTH * 0.5f, BTN_Y + BTN_H + 60.0f,
		"ENTER / CLICK  START", 0.42f, { 0.55f, 0.58f, 0.66f });

	Sprite_SetFilter(kSpriteFilter_Point);
	Sprite_Flush();
}