/*============================================================================
Contents   :  [player_visual.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/24
-----------------------------------------------------------------------------

============================================================================*/
#include <cmath>

#include "player_visual.h"
#include "texture.h"
#include "camera.h"

using namespace DirectX;

static constexpr int SHEET_CELL = 48;
static constexpr int SHEET_COLS = 4;   

static int g_tex = -1;

struct ClipInfo
{
	int   base_row = 0;
	int   frames = 4;      
	float frame_time = 0.12f;
	bool  directional = true;
	bool  loops = true;
};

static constexpr ClipInfo g_ClipInfo[PLAYER_CLIP_COUNT] =
{
	{  0, 4, 0.160f, true,  true  },   // IDLE  
	{  4, 8, 0.090f, true,  true  },   // RUN   
	{ 16, 4, 0.150f, false, false },   // DEATH 
};

static const ClipInfo& Info(PlayerClip clip)
{
	static constexpr ClipInfo fallback{};
	if (clip < 0 || clip >= PLAYER_CLIP_COUNT) { return fallback; }
	return g_ClipInfo[clip];
}

void PlayerVisual_Initialize()
{
	g_tex = Texture_Load(L"assets/textures/Wizard_sheet.png");
}

void PlayerVisual_Finalize()
{
	Texture_Release(g_tex);
	g_tex = -1;
}

int   PlayerVisual_GetFrameCount(PlayerClip clip) { return Info(clip).frames; }
float PlayerVisual_GetFrameTime(PlayerClip clip) { return Info(clip).frame_time; }
bool  PlayerVisual_IsLooping(PlayerClip clip) { return Info(clip).loops; }

int PlayerVisual_FacingFromDir(const Vector2& dir)
{
	if (fabsf(dir.x) > fabsf(dir.y))
	{
		return (dir.x > 0.0f) ? PLAYER_FACE_RIGHT : PLAYER_FACE_LEFT;
	}
	return (dir.y > 0.0f) ? PLAYER_FACE_DOWN : PLAYER_FACE_UP;
}

int PlayerVisual_FacingSticky(const Vector2& dir, int current_facing)
{
	constexpr float STICK = 1.35f;

	const float ax = fabsf(dir.x);
	const float ay = fabsf(dir.y);
	if (ax < 0.0001f && ay < 0.0001f) { return current_facing; }

	const bool holding_horizontal =
		(current_facing == PLAYER_FACE_LEFT || current_facing == PLAYER_FACE_RIGHT);

	const bool horizontal = holding_horizontal ? (ax * STICK >= ay)
		: (ax >= ay * STICK);

	if (horizontal)
	{
		return (dir.x > 0.0f) ? PLAYER_FACE_RIGHT : PLAYER_FACE_LEFT;
	}
	return (dir.y > 0.0f) ? PLAYER_FACE_DOWN : PLAYER_FACE_UP;
}

void PlayerVisual_Draw(PlayerClip clip, int frame, int facing,
	const Vector2& world_pos, float draw_size,
	const SpriteDrawParams& params)
{
	if (g_tex < 0) { return; }

	const ClipInfo& ci = Info(clip);

	const int f = (ci.frames > 0) ? (frame % ci.frames) : 0;
	const int face = (facing >= 0 && facing < PLAYER_FACE_COUNT) ? facing : 0;

	const int col = f % SHEET_COLS;
	const int block = f / SHEET_COLS;
	const int row = ci.directional
		? (ci.base_row + block * PLAYER_FACE_COUNT + face)
		: (ci.base_row + block);

	SpriteDrawParams p = params;
	p.flip_x = false;

	const float half = draw_size * 0.5f;

	Sprite_Draw(
		g_tex,
		Camera_WorldToScreenX(world_pos.x - half),
		Camera_WorldToScreenY(world_pos.y - half),
		draw_size, draw_size,
		static_cast<float>(col * SHEET_CELL),
		static_cast<float>(row * SHEET_CELL),
		SHEET_CELL, SHEET_CELL,
		p);
}