/*============================================================================
Contents   :  [boss_visual.cpp]

Author     : Chin Qing You
LastUpdate : 2026/09/04
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>

#include "boss_visual.h"
#include "texture.h"
#include "camera.h"
#include "draw_primitives.h"

using namespace DirectX;

enum BossSheet
{
	SHEET_SLIME1_IDLE,
	SHEET_SLIME1_RUN,
	SHEET_SLIME1_ATTACK,
	SHEET_SLIME1_DEATH,

	SHEET_SLIME3_IDLE,
	SHEET_SLIME3_RUN,
	SHEET_SLIME3_ATTACK,
	SHEET_SLIME3_DEATH,

	SHEET_SLIME2_IDLE,
	SHEET_SLIME2_RUN,
	SHEET_SLIME2_ATTACK,
	SHEET_SLIME2_DEATH,

	BOSS_SHEET_COUNT,
};

static const wchar_t* g_SheetFile[BOSS_SHEET_COUNT] =
{
	L"assets/textures/Slime1_Idle_full.png",
	L"assets/textures/Slime1_Run_full.png",
	L"assets/textures/Slime1_Attack_full.png",
	L"assets/textures/Slime1_Death_full.png",

	L"assets/textures/Slime3_Idle_full.png",
	L"assets/textures/Slime3_Run_full.png",
	L"assets/textures/Slime3_Attack_full.png",
	L"assets/textures/Slime3_Death_full.png",

	L"assets/textures/Slime2_Idle_full.png",
	L"assets/textures/Slime2_Run_full.png",
	L"assets/textures/Slime2_Attack_full.png",
	L"assets/textures/Slime2_Death_full.png",
};

static int g_SheetTex[BOSS_SHEET_COUNT];

struct BossClipInfo
{
	BossSheet sheet = SHEET_SLIME1_IDLE;
	int       frames = 1;
	int       cell = 64;
	float     frame_time = 0.09f;
};

//   basic (Slime1): idle 6, run 8, attack 10, death 10
//   laser (Slime3): idle 6, run 8, attack  9, death 10
//   wall  (Slime2): idle 6, run 8, attack 11, death 10
static constexpr BossClipInfo g_ClipInfo[BOSS_VARIANT_COUNT][BOSS_CLIP_COUNT] =
{
	// ---- BOSS_VARIANT_BASIC : Slime1 ----
	{
		{ SHEET_SLIME1_IDLE,   6, 64, 0.090f },
		{ SHEET_SLIME1_RUN,    8, 64, 0.090f },
		{ SHEET_SLIME1_ATTACK, 10, 64, 0.090f },
		{ SHEET_SLIME1_DEATH,  10, 64, 0.090f },
	},
	// ---- BOSS_VARIANT_LASER : Slime3 ----
	{
		{ SHEET_SLIME3_IDLE,   6, 64, 0.090f },
		{ SHEET_SLIME3_RUN,    8, 64, 0.090f },
		{ SHEET_SLIME3_ATTACK,  9, 64, 0.090f },
		{ SHEET_SLIME3_DEATH,  10, 64, 0.090f },
	},
	// ---- BOSS_VARIANT_WALL : Slime2 ----
	{
		{ SHEET_SLIME2_IDLE,   6, 64, 0.090f },
		{ SHEET_SLIME2_RUN,    8, 64, 0.090f },
		{ SHEET_SLIME2_ATTACK, 11, 64, 0.090f },
		{ SHEET_SLIME2_DEATH,  10, 64, 0.090f },
	},
};

static const BossClipInfo& Info(int variant, BossClip clip)
{
	static constexpr BossClipInfo fallback{};
	if (variant < 0 || variant >= BOSS_VARIANT_COUNT) { return fallback; }
	if (clip < 0 || clip >= BOSS_CLIP_COUNT) { return fallback; }
	return g_ClipInfo[variant][clip];
}

void BossVisual_Initialize()
{
	for (int i = 0; i < BOSS_SHEET_COUNT; i++)
	{
		g_SheetTex[i] = Texture_Load(g_SheetFile[i]);
	}
}

void BossVisual_Finalize()
{
	for (int i = 0; i < BOSS_SHEET_COUNT; i++)
	{
		Texture_Release(g_SheetTex[i]);
		g_SheetTex[i] = -1;
	}
}

int BossVisual_GetFrameCount(int variant, BossClip clip)
{
	return Info(variant, clip).frames;
}

float BossVisual_GetFrameTime(int variant, BossClip clip)
{
	return Info(variant, clip).frame_time;
}

int BossVisual_FacingFromDir(const Vector2& dir)
{
	if (fabsf(dir.x) > fabsf(dir.y))
	{
		return (dir.x > 0.0f) ? BOSS_FACE_RIGHT : BOSS_FACE_LEFT;
	}
	return (dir.y > 0.0f) ? BOSS_FACE_DOWN : BOSS_FACE_UP;
}

XMFLOAT3 BossVisual_ElementColor(ElementType element)
{
	switch (element)
	{
	case ELEMENT_FIRE:    return { 1.00f, 0.35f, 0.10f };
	case ELEMENT_ICE:     return { 0.30f, 0.70f, 1.00f };
	case ELEMENT_THUNDER: return { 1.00f, 0.90f, 0.20f };
	default:              return { 1.00f, 1.00f, 1.00f };
	}
}

void BossVisual_DrawShadow(const Vector2& world_pos, float radius,
	float jump_height, float arc_height)
{
	if (jump_height <= 1.0f) { return; }
	if (arc_height <= 0.0f) { return; }

	const float s = 1.0f - (jump_height / arc_height) * 0.45f;
	DrawPrim_Circle(world_pos, radius * s, { 0.0f, 0.0f, 0.0f }, 0.30f);
}

void BossVisual_DrawBody(int variant, BossClip clip,
	int frame, int facing,
	const Vector2& world_pos, float draw_size,
	float jump_height,
	const SpriteDrawParams& params)
{
	const BossClipInfo& ci = Info(variant, clip);

	const int sheet_index = static_cast<int>(ci.sheet);
	if (sheet_index < 0 || sheet_index >= BOSS_SHEET_COUNT) { return; }

	const int tex = g_SheetTex[sheet_index];
	if (tex < 0) { return; }

	// death holds on its last frame; everything else loops
	int col = 0;
	if (ci.frames > 0)
	{
		col = (clip == BOSS_CLIP_DEATH)
			? std::min(frame, ci.frames - 1)
			: (frame % ci.frames);
	}

	const int row = (facing >= 0 && facing < BOSS_FACE_COUNT) ? facing : 0;

	const float half = draw_size * 0.5f;

	Sprite_Draw(
		tex,
		Camera_WorldToScreenX(world_pos.x - half),
		Camera_WorldToScreenY(world_pos.y - half - jump_height),
		draw_size, draw_size,
		static_cast<float>(col * ci.cell),
		static_cast<float>(row * ci.cell),
		ci.cell, ci.cell,
		params);
}

void BossVisual_DrawShield(const Vector2& world_pos, float radius,
	float jump_height, ElementType element)
{
	const XMFLOAT3 col = BossVisual_ElementColor(element);
	const Vector2  at = world_pos - Vector2{ 0.0f, jump_height };

	DrawPrim_Ring(at, radius + 18.0f, 4.0f, col, 0.90f);
	DrawPrim_Ring(at, radius + 10.0f, 2.0f, col, 0.45f);
}