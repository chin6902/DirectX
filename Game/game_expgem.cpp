/*============================================================================
Contents   :  [game_expgem.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/17
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>

#include "game_expgem.h"
#include "game_player.h"
#include "game_progress.h"
#include "draw_primitives.h"
#include "game_audio.h"

using namespace DirectX;

static constexpr int   GEM_MAX = 750;
static constexpr float GEM_SIZE = 6.5f;   
static constexpr float GEM_COLLECT_RADIUS = 22.0f;  
static constexpr float GEM_ACCEL = 1500.0f;
static constexpr float GEM_SPEED_MAX = 900.0f;
static constexpr float GEM_BOB_RATE = 3.0f;
static constexpr float GEM_BOB_AMOUNT = 3.0f;

static constexpr float GEM_LIFETIME = 60.0f;
static constexpr float GEM_FADE_TIME = 3.0f;

struct ExpGem
{
	Vector2 pos;
	float speed;
	float bob_phase;
	float life;
	int xp_value;
	bool attracted;
	bool active;
};

static ExpGem g_Gems[GEM_MAX]{}; 
static int g_ActiveCount = 0;

void GameExpGem_Initialize()
{
	for (ExpGem& gem : g_Gems)
	{
		gem.active = false;
	}

	g_ActiveCount = 0;
}

void GameExpGem_Finalize()
{
	for (ExpGem& gem : g_Gems)
	{
		gem.active = false;
	}

	g_ActiveCount = 0;
}

void GameExpGem_Spawn(const Vector2& pos, int xp_value)
{
	if (xp_value <= 0) { return; }

	int slot = -1;

	for (int i = 0; i < GEM_MAX; i++)
	{
		if (!g_Gems[i].active) { slot = i; break; }
	}

	if (slot < 0)
	{
		const Vector2 player = GamePlayer_GetPos();
		float worst = -1.0f;

		for (int i = 0; i < GEM_MAX; i++)
		{
			const float d = (g_Gems[i].pos - player).LengthSq();
			if (d > worst) { worst = d; slot = i; }
		}
	}
	else
	{
		g_ActiveCount++;   
	}

	ExpGem& g = g_Gems[slot];
	g.pos = pos;
	g.speed = 0.0f;
	g.bob_phase = (rand() % 628) * 0.01f;
	g.life = GEM_LIFETIME;
	g.xp_value = xp_value;
	g.attracted = false;
	g.active = true;
}

void GameExpGem_Update(float delta_time)
{
	const Vector2 player_pos = GamePlayer_GetPos();

	const float pickup = GameProgress_GetStat(STAT_PICKUP_RANGE);
	const float pickup_sq = pickup * pickup;

	for (ExpGem& g : g_Gems)
	{
		if (!g.active) { continue; }

		g.life -= delta_time;
		if (g.life <= 0.0f)
		{
			g.active = false;
			g_ActiveCount--;
			continue;
		}

		const Vector2 to_player = player_pos - g.pos;
		const float dist_sq = to_player.LengthSq();

		if (dist_sq < GEM_COLLECT_RADIUS * GEM_COLLECT_RADIUS)
		{
			GameProgress_AddXP(g.xp_value);
			GameAudio_Play(SND_PICKUP);
			g.active = false;
			g_ActiveCount--;
			continue;
		}

		if (!g.attracted && dist_sq < pickup_sq)
		{
			g.attracted = true;
		}

		if (g.attracted)
		{
			g.speed = std::min(g.speed + GEM_ACCEL * delta_time, GEM_SPEED_MAX);
			g.pos += Vector2_Normalize(to_player) * (g.speed * delta_time);
		}
		else
		{
			g.bob_phase += GEM_BOB_RATE * delta_time;
		}
	}
}

void GameExpGem_Draw()
{
	for (const ExpGem& g : g_Gems)
	{
		if (!g.active) { continue; }

		const float bob = g.attracted ? 0.0f : sinf(g.bob_phase) * GEM_BOB_AMOUNT;

		float alpha = 1.0f;
		if (g.life < GEM_FADE_TIME)
		{
			alpha = (fmodf(g.life, 0.35f) < 0.18f) ? 0.25f : 0.9f;
		}

		const Vector2 p{ g.pos.x, g.pos.y + bob };

		static constexpr float GEM_ANGLE = 0.7853982f;

		DrawPrim_Rect(p, GEM_SIZE * 1.7f, GEM_SIZE * 1.7f, GEM_ANGLE,
			{ 0.55f, 0.85f, 1.0f }, 0.25f * alpha);         
		DrawPrim_Rect(p, GEM_SIZE, GEM_SIZE, GEM_ANGLE,
			{ 1.0f, 1.0f, 1.0f }, alpha);
	}
}

int GameExpGem_GetActiveCount()
{
	return g_ActiveCount;
}
