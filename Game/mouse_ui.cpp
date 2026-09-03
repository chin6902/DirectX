/*============================================================================
Contents   :  [mouse_ui.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/28
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>

#include "mouse_ui.h"
#include "texture.h"
#include "sprite.h"
#include "input_mouse.h"
#include "element.h"
#include "config.h"

using namespace DirectX;

static constexpr int   CELL_W = 128;
static constexpr int   CELL_H = 176;
static constexpr float DRAW_W = 74.0f;
static constexpr float DRAW_H = DRAW_W * (static_cast<float>(CELL_H) / CELL_W);

static constexpr float POS_X = SCREEN_WIDTH - DRAW_W - 42.0f;
static constexpr float POS_Y = SCREEN_HEIGHT - DRAW_H - 42.0f;

static constexpr float IDLE_ALPHA = 0.30f;  
static constexpr float HELD_ALPHA = 0.95f;
static constexpr float FADE_RATE = 6.0f;    

enum MousePart
{
	PART_OUTLINE,
	PART_LEFT,
	PART_WHEEL,
	PART_RIGHT,
	PART_COUNT,
};

static constexpr ElementType g_PartElement[PART_COUNT] =
{
	ELEMENT_FIRE,      
	ELEMENT_FIRE,
	ELEMENT_ICE,
	ELEMENT_THUNDER,
};

static constexpr MouseButton g_PartButton[PART_COUNT] =
{
	MOUSE_BUTTON_LEFT,   
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_MIDDLE,
	MOUSE_BUTTON_RIGHT,
};

static int   g_texture = -1;
static float g_Lit[PART_COUNT]{};   

static XMFLOAT3 ElementColor(ElementType e)
{
	switch (e)
	{
	case ELEMENT_FIRE:    return { 1.00f, 0.35f, 0.10f };
	case ELEMENT_ICE:     return { 0.30f, 0.70f, 1.00f };
	case ELEMENT_THUNDER: return { 1.00f, 0.90f, 0.20f };
	default:              return { 1.00f, 1.00f, 1.00f };
	}
}

void MouseUI_Initialize()
{
	g_texture = Texture_Load(L"assets/textures/mouse_ui.png");
	for (float& v : g_Lit) { v = 0.0f; }
}

void MouseUI_Finalize()
{
	Texture_Release(g_texture);
	g_texture = -1;
}

void MouseUI_Update(float delta_time)
{
	const float step = FADE_RATE * delta_time;

	for (int p = PART_LEFT; p < PART_COUNT; p++)
	{
		const bool held = InputMouse_IsPress(g_PartButton[p]);

		if (held) { g_Lit[p] = 1.0f; }
		else { g_Lit[p] = std::max(0.0f, g_Lit[p] - step); }
	}
}

static void DrawPart(int part, const XMFLOAT3& color, float alpha)
{
	if (alpha <= 0.0f) { return; }

	SpriteDrawParams p;
	p.color = color;
	p.alpha = alpha;

	Sprite_Draw(g_texture, POS_X, POS_Y, DRAW_W, DRAW_H,
		static_cast<float>(part * CELL_W), 0.0f,
		CELL_W, CELL_H, p);
}

void MouseUI_Draw()
{
	if (g_texture < 0) { return; }

	Sprite_SetFilter(kSpriteFilter_Linear);

	for (int p = PART_LEFT; p < PART_COUNT; p++)
	{
		DrawPart(p, ElementColor(g_PartElement[p]), g_Lit[p] * HELD_ALPHA);
	}

	float any = 0.0f;
	for (int p = PART_LEFT; p < PART_COUNT; p++) { any = std::max(any, g_Lit[p]); }

	DrawPart(PART_OUTLINE, { 0.85f, 0.88f, 0.95f },
		IDLE_ALPHA + (0.65f * any));

	Sprite_SetFilter(kSpriteFilter_Point);
}