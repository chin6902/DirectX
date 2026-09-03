/*============================================================================
Contents   :  [enemy_visual.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/24
-----------------------------------------------------------------------------

============================================================================*/
#include <cmath>

#include "enemy_visual.h"
#include "texture.h"
#include "camera.h"

using namespace DirectX;

enum EnemySheet
{
	SHEET_ORC1_RUN,
	SHEET_ORC1_ATTACK,
	SHEET_ORC2_WALK,
	SHEET_ORC2_ATTACK,
	SHEET_WONWON,
	SHEET_NONE,
	SHEET_COUNT = SHEET_NONE,
};

static const wchar_t* g_SheetFile[SHEET_COUNT] =
{
	L"assets/textures/orc1_run_full.png",
	L"assets/textures/orc1_attack_full.png",
	L"assets/textures/orc2_walk_full.png",
	L"assets/textures/orc2_attack_full.png",
	L"assets/textures/wonwon.png",
};

static int g_SheetTex[SHEET_COUNT] = { -1, -1, -1, -1, -1 };

enum SheetLayout
{
	LAYOUT_DULR,    
	LAYOUT_DLU_M,  
};

struct ClipInfo
{
	EnemySheet  sheet = SHEET_NONE;
	int         frames = 1;        
	int         cell = 64;
	float       frame_time = 0.10f;
	SheetLayout layout = LAYOUT_DULR;
	SoundId     cycle_sound = SND_NONE;
};

static constexpr ClipInfo g_ClipInfo[ENEMY_TYPE_COUNT][ENEMY_CLIP_COUNT] =
{
	// ---- CHASER: orc1 run, 8 frames, 4 facing rows ----
	{
		{ SHEET_ORC1_RUN,    8, 64, 0.085f, LAYOUT_DULR },   // MOVE
		{ SHEET_ORC1_ATTACK, 6, 64, 0.090f, LAYOUT_DULR },   // ATTACK
	},
	// ---- ORBITER: wonwon, 4 frames per direction, 3 rows + mirror ----
	{
		{ SHEET_WONWON, 4, 32, 0.110f, LAYOUT_DLU_M },
		{ SHEET_WONWON, 4, 32, 0.110f, LAYOUT_DLU_M },
	},
	// ---- ELITE: orc2 walks, orc2 attacks while dashing ----
	{
		{ SHEET_ORC2_WALK,   6, 64, 0.110f, LAYOUT_DULR },
		{ SHEET_ORC2_ATTACK, 8, 64, 0.055f, LAYOUT_DULR , SND_ELITE_SWING}, 
	},
	// ---- DUMMY: same orc as the chaser, facing locked at spawn ----
	{
		{ SHEET_ORC1_RUN, 8, 64, 0.130f, LAYOUT_DULR },
		{ SHEET_ORC1_RUN, 8, 64, 0.130f, LAYOUT_DULR },   
	},
};

static const ClipInfo& Info(EnemyType type, EnemyClip clip)
{
	static constexpr ClipInfo fallback{};
	if (type < 0 || type >= ENEMY_TYPE_COUNT) { return fallback; }
	if (clip < 0 || clip >= ENEMY_CLIP_COUNT) { return fallback; }
	return g_ClipInfo[type][clip];
}

static void ResolveRow(SheetLayout layout, int facing, int& out_row, bool& out_mirror)
{
	out_mirror = false;

	if (layout == LAYOUT_DLU_M)
	{
		switch (facing)
		{
		case ENEMY_FACE_DOWN:  out_row = 0; break;
		case ENEMY_FACE_LEFT:  out_row = 1; break;
		case ENEMY_FACE_UP:    out_row = 2; break;
		case ENEMY_FACE_RIGHT: out_row = 1; out_mirror = true; break;
		default:               out_row = 0; break;
		}
		return;
	}

	out_row = (facing >= 0 && facing < ENEMY_FACE_COUNT) ? facing : 0;
}

void EnemyVisual_Initialize()
{
	for (int i = 0; i < SHEET_COUNT; i++)
	{
		g_SheetTex[i] = Texture_Load(g_SheetFile[i]);
	}
}

void EnemyVisual_Finalize()
{
	for (int i = 0; i < SHEET_COUNT; i++)
	{
		Texture_Release(g_SheetTex[i]);
		g_SheetTex[i] = -1;
	}
}

int EnemyVisual_GetFrameCount(EnemyType type, EnemyClip clip)
{
	return Info(type, clip).frames;
}

float EnemyVisual_GetFrameTime(EnemyType type, EnemyClip clip)
{
	return Info(type, clip).frame_time;
}

int EnemyVisual_FacingFromDir(const Vector2& dir)
{
	if (fabsf(dir.x) > fabsf(dir.y))
	{
		return (dir.x > 0.0f) ? ENEMY_FACE_RIGHT : ENEMY_FACE_LEFT;
	}
	return (dir.y > 0.0f) ? ENEMY_FACE_DOWN : ENEMY_FACE_UP;
}

SoundId EnemyVisual_GetCycleSound(EnemyType type, EnemyClip clip)
{
	return Info(type, clip).cycle_sound;
}

void EnemyVisual_Draw(EnemyType type, EnemyClip clip,
	int frame, int facing,
	const Vector2& world_pos, float draw_size,
	const SpriteDrawParams& params)
{
	const ClipInfo& ci = Info(type, clip);

	const int sheet_index = static_cast<int>(ci.sheet);
	if (sheet_index < 0 || sheet_index >= SHEET_COUNT) { return; }

	const int tex = g_SheetTex[sheet_index];
	if (tex < 0) { return; }

	const int col = (ci.frames > 0) ? (frame % ci.frames) : 0;

	int  row = 0;
	bool mirror = false;
	ResolveRow(ci.layout, facing, row, mirror);

	SpriteDrawParams p = params;
	p.flip_x = mirror;

	const float half = draw_size * 0.5f;

	Sprite_Draw(
		tex,
		Camera_WorldToScreenX(world_pos.x - half),
		Camera_WorldToScreenY(world_pos.y - half),
		draw_size, draw_size,
		static_cast<float>(col * ci.cell),
		static_cast<float>(row * ci.cell),
		ci.cell, ci.cell,
		p);
}