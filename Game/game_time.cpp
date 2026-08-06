/*============================================================================
Contents   :  [game_time.cpp]

Author     : Chin Qing You
LastUpdate : 2026/07/31
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>

#include "game_time.h"

static float g_TimeScale = 1.0f;
static float g_HitstopTimer = 0.0f;

void GameTime_Initialize()
{
    g_TimeScale = 1.0f;
    g_HitstopTimer = 0.0f;
}

float GameTime_Scale(float raw_delta_time)
{
	if (g_HitstopTimer > 0.0f)
	{
		g_HitstopTimer -= raw_delta_time;
		return 0.0f;                   
	}

	return raw_delta_time * g_TimeScale;
}

void GameTime_SetScale(float scale)
{
	g_TimeScale = std::max(scale, 0.0f);
}

float GameTime_GetScale()
{
	return g_TimeScale;
}

void GameTime_Hitstop(float duration)
{
	g_HitstopTimer = std::max(g_HitstopTimer, duration);
}


