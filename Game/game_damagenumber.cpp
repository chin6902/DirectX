/*============================================================================
Contents   :  [game_damagenumber.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/21
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "game_damagenumber.h"
#include "game_text.h"
#include "camera.h"
#include "config.h"

using namespace DirectX;

static constexpr int   NUMBER_MAX = 64;

static constexpr float NUMBER_LIFE = 0.85f;
static constexpr float MERGE_WINDOW = 0.45f;   
static constexpr float RISE_SPEED = 90.0f;   
static constexpr float RISE_DAMPING = 2.6f;    
static constexpr float DRIFT_MAX = 26.0f;

static constexpr float SCALE_BASE = 0.42f;
static constexpr float SCALE_POP = 0.22f;   
static constexpr float POP_TIME = 0.12f;
static constexpr float MERGE_POP = 0.10f;   

struct DamageNumber
{
	Vector2  pos;
	Vector2  vel;
	float    life;
	float    merge_left;   
	float    pop_timer;
	float    value;
	int      target_id;
	XMFLOAT3 color;
	bool     active;
};

static DamageNumber g_Numbers[NUMBER_MAX]{};
static int          g_ActiveCount = 0;

void GameDamageNumber_Initialize()
{
	for (DamageNumber& n : g_Numbers) { n.active = false; }
	g_ActiveCount = 0;
}

void GameDamageNumber_Finalize()
{
	GameDamageNumber_Initialize();
}

int GameDamageNumber_GetActiveCount() { return g_ActiveCount; }

void GameDamageNumber_Spawn(const Vector2& world_pos, float damage, int target_id, const XMFLOAT3& color)
{
	if (damage <= 0.0f) { return; }


	if (target_id != 0)
	{
		for (DamageNumber& n : g_Numbers)
		{
			if (!n.active || n.merge_left <= 0.0f || n.target_id != target_id) { continue; }

			n.value += damage;
			n.life = NUMBER_LIFE;      
			n.merge_left = MERGE_WINDOW;
			n.pop_timer = MERGE_POP;        
			n.pos = world_pos;      
			return;
		}
	}

	DamageNumber* slot = nullptr;

	for (DamageNumber& n : g_Numbers)
	{
		if (!n.active) { slot = &n; break; }
	}

	if (slot == nullptr)
	{
		float shortest = NUMBER_LIFE + 1.0f;
		for (DamageNumber& n : g_Numbers)
		{
			if (n.life < shortest) { shortest = n.life; slot = &n; }
		}
		if (slot == nullptr) { return; }
	}
	else
	{
		g_ActiveCount++;
	}

	const float drift = ((rand() % 200) - 100) * 0.01f * DRIFT_MAX;

	slot->pos = world_pos;
	slot->vel = { drift, -RISE_SPEED };
	slot->life = NUMBER_LIFE;
	slot->merge_left = MERGE_WINDOW;
	slot->pop_timer = POP_TIME;
	slot->value = damage;
	slot->target_id = target_id;
	slot->color = color;
	slot->active = true;
}

void GameDamageNumber_Update(float delta_time)
{
	for (DamageNumber& n : g_Numbers)
	{
		if (!n.active) { continue; }

		n.life -= delta_time;
		if (n.life <= 0.0f)
		{
			n.active = false;
			g_ActiveCount--;
			continue;
		}

		if (n.merge_left > 0.0f) { n.merge_left -= delta_time; }
		if (n.pop_timer > 0.0f) { n.pop_timer -= delta_time; }

		n.pos += n.vel * delta_time;
		n.vel *= std::max(0.0f, 1.0f - RISE_DAMPING * delta_time);
	}
}

void GameDamageNumber_Draw()
{
	for (const DamageNumber& n : g_Numbers)
	{
		if (!n.active) { continue; }

		const float t = n.life / NUMBER_LIFE;          

		const float alpha = std::min(1.0f, t * 3.0f);

		float scale = SCALE_BASE;
		if (n.pop_timer > 0.0f)
		{
			scale += SCALE_POP * (n.pop_timer / POP_TIME);
		}

		char text[16];
		if (fabsf(n.value - roundf(n.value)) < 0.05f)
		{
			snprintf(text, sizeof(text), "%.0f", n.value);
		}
		else
		{
			snprintf(text, sizeof(text), "%.1f", n.value);
		}

		const float sx = Camera_WorldToScreenX(n.pos.x);
		const float sy = Camera_WorldToScreenY(n.pos.y);

		if (sx < -80.0f || sx > SCREEN_WIDTH + 80.0f
			|| sy < -80.0f || sy > SCREEN_HEIGHT + 80.0f)
		{
			continue;
		}

		GameText_DrawCentered(sx + 2.0f, sy + 2.0f, text, scale,
			{ 0.0f, 0.0f, 0.0f }, alpha * 0.7f);
		GameText_DrawCentered(sx, sy, text, scale, n.color, alpha);
	}
}