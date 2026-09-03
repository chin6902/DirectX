/*============================================================================
Contents   :  [player_visual.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/24
-----------------------------------------------------------------------------
One sheet, one table. A clip is a BASE ROW plus a frame count: the four
facing rows follow the base row in the order down, up, left, right, so a
clip is fully described by where its block starts.
============================================================================*/
#include <cmath>

#include "player_visual.h"
#include "texture.h"
#include "camera.h"

using namespace DirectX;

static constexpr int SHEET_CELL = 48;
static constexpr int SHEET_COLS = 4;   // frames per row before the clip wraps
// onto the next block of facing rows

static int g_tex = -1;

// A clip is a BLOCK of PLAYER_FACE_COUNT rows starting at base_row, in the
// order down, up, left, right. A clip longer than SHEET_COLS frames simply
// continues on the NEXT block: run is 8 frames, so frames 0-3 live in rows
// 4-7 and frames 4-7 live in rows 8-11.
//
// A non-directional clip is ONE row: death is drawn the same way whichever
// way the wizard was facing.
struct ClipInfo
{
	int   base_row = 0;
	int   frames = 4;      // total frames per direction
	float frame_time = 0.12f;
	bool  directional = true;
	bool  loops = true;
};

static constexpr ClipInfo g_ClipInfo[PLAYER_CLIP_COUNT] =
{
	{  0, 4, 0.160f, true,  true  },   // IDLE  - rows 0-3, slower, reads as breathing
	{  4, 8, 0.090f, true,  true  },   // RUN   - rows 4-7 then 8-11
	{ 16, 4, 0.150f, false, false },   // DEATH - row 16 only, holds last frame
	// rows 12-15 are a cast/attack pose if a third clip is ever wanted
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
	// How much the other axis has to beat the current one by before the
	// facing flips. 1.0 would be no bias at all; higher is stickier.
	constexpr float STICK = 1.35f;

	const float ax = fabsf(dir.x);
	const float ay = fabsf(dir.y);
	if (ax < 0.0001f && ay < 0.0001f) { return current_facing; }

	const bool holding_horizontal =
		(current_facing == PLAYER_FACE_LEFT || current_facing == PLAYER_FACE_RIGHT);

	// Already sideways: keep it unless the vertical clearly wins, and
	// vice versa.
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

	// Wrap rather than trust the caller: a clip switch can arrive carrying
	// a frame index from the previous, longer clip.
	const int f = (ci.frames > 0) ? (frame % ci.frames) : 0;
	const int face = (facing >= 0 && facing < PLAYER_FACE_COUNT) ? facing : 0;

	// Every SHEET_COLS frames the clip moves down one whole block of
	// facing rows, so the facing offset is added AFTER the block offset.
	const int col = f % SHEET_COLS;
	const int block = f / SHEET_COLS;
	const int row = ci.directional
		? (ci.base_row + block * PLAYER_FACE_COUNT + face)
		: (ci.base_row + block);

	// The sheet already carries a real right-facing row, so nothing is
	// mirrored here - flipping would break it.
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