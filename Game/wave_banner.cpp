/*============================================================================
Contents   :  [wave_banner.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/27
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "wave_banner.h"
#include "enemy_spawner.h"
#include "game_text.h"
#include "game_ui.h"
#include "camera.h"
#include "config.h"

using namespace DirectX;

// --- timing ---
static constexpr float SLIDE_IN_TIME = 0.35f;
static constexpr float HOLD_TIME_NORMAL = 1.20f;
static constexpr float HOLD_TIME_BOSS = 2.10f;   
static constexpr float SLIDE_OUT_TIME = 0.35f;

// --- layout ---
static constexpr float BAND_H = 92.0f;
static constexpr float BAND_Y = SCREEN_HEIGHT * 0.5f - BAND_H * 0.5f - 60.0f;
static constexpr float SLIDE_FROM = 260.0f;   

// --- boss alarm ---
static constexpr float PULSE_RATE = 9.0f;
static constexpr float PULSE_MAX_ALPHA = 0.22f;
static constexpr float BOSS_SHAKE = 14.0f;
static constexpr float BOSS_SHAKE_TIME = 0.45f;

enum BannerState
{
	BANNER_HIDDEN,
	BANNER_SLIDE_IN,
	BANNER_HOLD,
	BANNER_SLIDE_OUT,
};

static BannerState g_State = BANNER_HIDDEN;
static float g_Timer = 0.0f;
static bool  g_IsBoss = false;
static int   g_ShownWave = 0;   
static int   g_LastWave = 0;   
static float g_PulseTime = 0.0f;

void WaveBanner_Initialize()
{
	g_State = BANNER_HIDDEN;
	g_Timer = 0.0f;
	g_IsBoss = false;
	g_PulseTime = 0.0f;

	g_ShownWave = 0;
	g_LastWave = 0;
}

void WaveBanner_Finalize()
{
	g_State = BANNER_HIDDEN;
}

static void Trigger(int wave, bool is_boss)
{
	g_ShownWave = wave;
	g_IsBoss = is_boss;
	g_State = BANNER_SLIDE_IN;
	g_Timer = 0.0f;

	if (is_boss) { Camera_Shake(BOSS_SHAKE, BOSS_SHAKE_TIME); }
}

void WaveBanner_Update(float delta_time)
{
	g_PulseTime += delta_time;

	const int wave = EnemySpawner_GetWaveNumber();
	if (wave != g_LastWave && !EnemySpawner_IsFinished())
	{
		g_LastWave = wave;
		Trigger(wave, EnemySpawner_IsBossWave());
	}

	if (g_State == BANNER_HIDDEN) { return; }

	g_Timer += delta_time;

	const float hold = g_IsBoss ? HOLD_TIME_BOSS : HOLD_TIME_NORMAL;

	switch (g_State)
	{
	case BANNER_SLIDE_IN:
		if (g_Timer >= SLIDE_IN_TIME) { g_State = BANNER_HOLD; g_Timer = 0.0f; }
		break;
	case BANNER_HOLD:
		if (g_Timer >= hold) { g_State = BANNER_SLIDE_OUT; g_Timer = 0.0f; }
		break;
	case BANNER_SLIDE_OUT:
		if (g_Timer >= SLIDE_OUT_TIME) { g_State = BANNER_HIDDEN; g_Timer = 0.0f; }
		break;
	default:
		break;
	}
}

static float EaseOut(float t)
{
	const float inv = 1.0f - t;
	return 1.0f - inv * inv * inv;
}

static float EaseIn(float t)
{
	return t * t * t;
}

static void SlideState(float& out_offset, float& out_alpha)
{
	switch (g_State)
	{
	case BANNER_SLIDE_IN:
	{
		const float t = std::clamp(g_Timer / SLIDE_IN_TIME, 0.0f, 1.0f);
		out_offset = -SLIDE_FROM * (1.0f - EaseOut(t));
		out_alpha = t;
		break;
	}
	case BANNER_HOLD:
		out_offset = 0.0f;
		out_alpha = 1.0f;
		break;
	case BANNER_SLIDE_OUT:
	{
		const float t = std::clamp(g_Timer / SLIDE_OUT_TIME, 0.0f, 1.0f);
		out_offset = SLIDE_FROM * EaseIn(t);
		out_alpha = 1.0f - t;
		break;
	}
	default:
		out_offset = 0.0f;
		out_alpha = 0.0f;
		break;
	}
}

void WaveBanner_Draw()
{
	if (g_State == BANNER_HIDDEN) { return; }

	float offset = 0.0f;
	float alpha = 1.0f;
	SlideState(offset, alpha);

	const float cx = SCREEN_WIDTH * 0.5f + offset;

	if (g_IsBoss)
	{
		const float pulse = fabsf(sinf(g_PulseTime * PULSE_RATE));
		GameUI_DrawScreenRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT,
			{ 0.85f, 0.10f, 0.10f }, pulse * PULSE_MAX_ALPHA * alpha);
	}

	const XMFLOAT3 band_col = g_IsBoss ? XMFLOAT3{ 0.28f, 0.03f, 0.04f }
	: XMFLOAT3{ 0.06f, 0.07f, 0.10f };
	GameUI_DrawScreenRect(0.0f, BAND_Y, (float)SCREEN_WIDTH, BAND_H,
		band_col, 0.82f * alpha);

	// thin rules top and bottom
	const XMFLOAT3 rule_col = g_IsBoss ? XMFLOAT3{ 1.00f, 0.25f, 0.25f }
	: XMFLOAT3{ 0.45f, 0.85f, 1.00f };
	GameUI_DrawScreenRect(0.0f, BAND_Y - 2.0f, (float)SCREEN_WIDTH, 2.0f,
		rule_col, 0.9f * alpha);
	GameUI_DrawScreenRect(0.0f, BAND_Y + BAND_H, (float)SCREEN_WIDTH, 2.0f,
		rule_col, 0.9f * alpha);

	// --- text ---
	char line[48];

	if (g_IsBoss)
	{
		const float text_pulse = 0.75f + 0.25f * sinf(g_PulseTime * PULSE_RATE + 1.6f);
		GameText_DrawCentered(cx, BAND_Y + 14.0f, "! W A R N I N G !", 0.95f,
			{ 1.0f, 0.30f * text_pulse + 0.20f, 0.25f * text_pulse });

		snprintf(line, sizeof(line), "WAVE %d   BOSS INCOMING", g_ShownWave);
		GameText_DrawCentered(cx, BAND_Y + 60.0f, line, 0.42f,
			{ 1.0f, 0.80f, 0.80f });
	}
	else
	{
		snprintf(line, sizeof(line), "WAVE %d / %d",
			g_ShownWave, EnemySpawner_GetWaveCount());
		GameText_DrawCentered(cx, BAND_Y + 30.0f, line, 1.00f,
			{ 1.0f, 1.0f, 1.0f });
	}
}

void WaveBanner_DrawCounter()
{
	char line[32];

	if (EnemySpawner_IsFinished())
	{
		GameText_Draw(40.0f, 34.0f, "ALL WAVES CLEAR", 0.40f, { 0.65f, 1.0f, 0.70f });
		return;
	}

	snprintf(line, sizeof(line), "WAVE %d / %d",
		EnemySpawner_GetWaveNumber(), EnemySpawner_GetWaveCount());

	const bool boss = EnemySpawner_IsBossWave();
	GameText_Draw(40.0f, 34.0f, line, 0.40f,
		boss ? XMFLOAT3{ 1.00f, 0.45f, 0.45f } : XMFLOAT3{ 0.80f, 0.85f, 0.95f });

	const float remaining = EnemySpawner_GetWaveTimeout();
	if (remaining > 0.0f)
	{
		const int mins = static_cast<int>(remaining) / 60;
		const int secs = static_cast<int>(remaining) % 60;
		snprintf(line, sizeof(line), "%02d:%02d", mins, secs);

		const bool urgent = (remaining < 10.0f);
		GameText_DrawCentered((static_cast<float>(SCREEN_WIDTH) / 2.0f), 62.0f, line, 0.75f,
			urgent ? XMFLOAT3{ 1.00f, 0.45f, 0.40f } : XMFLOAT3{ 0.85f, 0.88f, 0.95f });
	}
}