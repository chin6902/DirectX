/*============================================================================
Contents   :  [flipbook_animation.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/22
-----------------------------------------------------------------------------
// animation one shot, start and stop from certain animation frame
============================================================================*/
#include "flipbook_animation.h"
#include "sprite.h"

struct FlipBookAnimation
{
	int texture_id;
	int pattern_width;
	int pattern_height;

	int pattern_count_max;
	int pattern_column_count_max;
	float pattern_update_time;

	int pattern_current_count;
	float pattern_accumulated_time;
};

static constexpr int ANIMATION_MAX{ 128 };
static FlipBookAnimation g_Animations[ANIMATION_MAX]{};

void FlipBookAnimation_Initialize()
{
	for (FlipBookAnimation& a : g_Animations)
	{
		a.pattern_count_max = 0; 
	}
}

void FlipBookAnimation_Finalize()
{
}

int FlipBookAnimation_Create(int texture_id, int pattern_width, int pattern_height, int pattern_count_max, int pattern_column_count_max, float pattern_update_time)
{
	for (int i = 0; i < ANIMATION_MAX; i++)
	{
		if (g_Animations[i].pattern_count_max != 0)
		{
			continue;
		}

		g_Animations[i].texture_id = texture_id;
		g_Animations[i].pattern_width = pattern_width;
		g_Animations[i].pattern_height = pattern_height;
		g_Animations[i].pattern_count_max = pattern_count_max;
		g_Animations[i].pattern_column_count_max = pattern_column_count_max;
		g_Animations[i].pattern_update_time = pattern_update_time;
		g_Animations[i].pattern_current_count = 0;
		g_Animations[i].pattern_accumulated_time = 0.0f;

		return i;
	}

	return -1;
}

void FlipBookAnimation_Destroy(int animation_id)
{
	if (animation_id < 0 || animation_id >= ANIMATION_MAX)
	{
		return;
	}

	g_Animations[animation_id].pattern_count_max = 0;
}


void FlipBookAnimation_Update(float delta_time)
{
	for (int i = 0; i < ANIMATION_MAX; i++)
	{
		if (g_Animations[i].pattern_count_max == 0)
		{
			continue;
		}

		g_Animations[i].pattern_accumulated_time += delta_time;

		if (g_Animations[i].pattern_accumulated_time >= g_Animations[i].pattern_update_time)
		{
			g_Animations[i].pattern_accumulated_time -= g_Animations[i].pattern_update_time;
			g_Animations[i].pattern_current_count++;

			if (g_Animations[i].pattern_current_count >= g_Animations[i].pattern_count_max)
			{
				g_Animations[i].pattern_current_count = 0;
			}
		}
	}
}

void FlipBookAnimation_Draw(int animation_id, float x, float y)
{
	FlipBookAnimation_Draw(animation_id, x, y, { 1.0f, 1.0f, 1.0f, 1.0f });
}
/*
void FlipBookAnimation_Draw(int animation_id, float x, float y)
{
	FlipBookAnimation* p = &g_Animations[animation_id];
	int current_pattern_x = p->pattern_current_count % p->pattern_column_count_max;
	int current_pattern_y = p->pattern_current_count / p->pattern_column_count_max;

	Sprite_Draw(p->texture_id,
		x, y,
		p->pattern_width * current_pattern_x, p->pattern_height * current_pattern_y,
		p->pattern_width, p->pattern_height
	);
}
*/
void FlipBookAnimation_Draw(int animation_id, float x, float y, const DirectX::XMFLOAT4& color)
{
	if (animation_id < 0 || animation_id >= ANIMATION_MAX) return;

	const FlipBookAnimation& anim = g_Animations[animation_id];
	if (anim.pattern_count_max == 0) return; // Slot is not in use

	// Work out which row and column the current pattern lives in
	//   e.g. pattern 7 in a sheet with 5 columns = row 1, column 2
	int col = anim.pattern_current_count % anim.pattern_column_count_max;
	int row = anim.pattern_current_count / anim.pattern_column_count_max;

	float texture_x = static_cast<float>(col * anim.pattern_width);
	float texture_y = static_cast<float>(row * anim.pattern_height);

	// Reuse the full-featured Sprite_Draw that takes a source rect
	Sprite_Draw(
		anim.texture_id,
		x, y,
		static_cast<float>(anim.pattern_width),   // draw width  = one pattern cell
		static_cast<float>(anim.pattern_height),  // draw height = one pattern cell
		texture_x, texture_y,
		anim.pattern_width,
		anim.pattern_height,
		color
	);
}