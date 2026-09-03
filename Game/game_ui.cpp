/*============================================================================
Contents   :  [game_ui.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/24
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "game_ui.h"
#include "game_player.h"
#include "game_progress.h"
#include "game_item.h"
#include "player_charge.h"
#include "game_text.h"
#include "texture.h"
#include "sprite.h"
#include "config.h"

using namespace DirectX;

// --- health, bottom-left ---
static constexpr float HP_X = 40.0f;
static constexpr float HP_Y = SCREEN_HEIGHT - 78.0f;
static constexpr float HP_W = 360.0f;
static constexpr float HP_H = 26.0f;

// --- player bars stacked ABOVE the HP bar ---
static constexpr float BAR_GAP = 8.0f;
static constexpr float GUTTER = 36.0f;                      
static constexpr float ULT_BAR_H = 16.0f;
static constexpr float BUFF_H = 30.0f;
static constexpr float ULT_Y = HP_Y - BAR_GAP - ULT_BAR_H;  
static constexpr float BUFF_Y = ULT_Y - BAR_GAP - BUFF_H;    

// --- progress, top-right ---
static constexpr float PR_W = 300.0f;
static constexpr float PR_X = SCREEN_WIDTH - PR_W - 40.0f;
static constexpr float PR_Y = 34.0f;
static constexpr float XP_H = 14.0f;
static constexpr float PIP_H = 14.0f;
static constexpr float PIP_GAP = 6.0f;

static constexpr float GHOST_FALL_SPEED = 0.55f;
static constexpr float FLASH_MAX_ALPHA = 0.55f;

static int   g_white_texture_id = -1;
static float g_GhostFraction = 1.0f;

static float    g_FlashTimer = 0.0f;
static float    g_FlashMax = 0.0f;
static XMFLOAT3 g_FlashColor{ 1.0f, 1.0f, 1.0f };

static float    g_PulseTime = 0.0f;

static void DrawRect(float x, float y, float w, float h,
	const XMFLOAT3& color, float alpha = 1.0f)
{
	if (w <= 0.0f || h <= 0.0f) { return; }
	SpriteDrawParams p;
	p.color = color;
	p.alpha = alpha;
	Sprite_Draw(g_white_texture_id, x, y, w, h, p);
}

static XMFLOAT3 ElementColor(int e)
{
	switch (e)
	{
	case ELEMENT_FIRE:    return { 1.00f, 0.40f, 0.12f };
	case ELEMENT_ICE:     return { 0.30f, 0.70f, 1.00f };
	case ELEMENT_THUNDER: return { 1.00f, 0.88f, 0.25f };
	default:              return { 0.60f, 0.60f, 0.60f };
	}
}

static const char* ElementShortName(int e)
{
	switch (e)
	{
	case ELEMENT_FIRE:    return "FIRE";
	case ELEMENT_ICE:     return "ICE";
	case ELEMENT_THUNDER: return "THDR";
	default:              return "?";
	}
}

void GameUI_Initialize()
{
	g_white_texture_id = Texture_Load(L"assets/textures/white_debug.png", false);
	g_GhostFraction = 1.0f;
}

void GameUI_Finalize()
{
	Texture_Release(g_white_texture_id);
}

void GameUI_Flash(const XMFLOAT3& color, float duration)
{
	if (duration <= g_FlashTimer) { return; }
	g_FlashColor = color;
	g_FlashMax = duration;
	g_FlashTimer = duration;
}

void GameUI_Update(float delta_time)
{
	g_PulseTime += delta_time;
	if (g_FlashTimer > 0.0f) { g_FlashTimer -= delta_time; }

	const float target = GamePlayer_GetHPFraction();
	if (g_GhostFraction > target)
	{
		g_GhostFraction = std::max(target, g_GhostFraction - GHOST_FALL_SPEED * delta_time);
	}
	else
	{
		g_GhostFraction = target;
	}
}

// ============================================================================
// Draw
// ============================================================================
static void DrawHealthBar()
{
	const float fraction = GamePlayer_GetHPFraction();

	// backing + border
	DrawRect(HP_X - 3.0f, HP_Y - 3.0f, HP_W + 6.0f, HP_H + 6.0f, { 0.06f, 0.07f, 0.09f }, 0.90f);
	DrawRect(HP_X, HP_Y, HP_W, HP_H, { 0.16f, 0.10f, 0.12f }, 1.0f);

	// ghost
	DrawRect(HP_X, HP_Y, HP_W * g_GhostFraction, HP_H, { 0.85f, 0.75f, 0.35f }, 0.85f);

	// the real value
	XMFLOAT3 fill = { 0.85f, 0.20f, 0.22f };
	if (fraction > 0.6f) { fill = { 0.30f, 0.80f, 0.35f }; }
	else if (fraction > 0.3f) { fill = { 0.90f, 0.70f, 0.20f }; }
	DrawRect(HP_X, HP_Y, HP_W * fraction, HP_H, fill, 1.0f);

	char hp[32];
	snprintf(hp, sizeof(hp), "HP %d / %d", GamePlayer_GetHP(), GamePlayer_GetMaxHP());
	GameText_Draw(HP_X + 6.0f, HP_Y + HP_H + 8.0f, hp, 0.40f, { 0.85f, 0.87f, 0.92f });
}

// --- ULT meter, relocated to sit directly above the HP bar ------------------
static void DrawUltimateBar()
{
	const float ult = PlayerCharge_GetUltimateFraction();
	const XMFLOAT3 col = ElementColor(PlayerCharge_GetUltimateElement());
	const bool  ready = (ult > 0.85f);

	const float bar_x = HP_X + GUTTER;
	const float bar_w = HP_W - GUTTER;

	GameText_Draw(HP_X, ULT_Y - 1.0f, "ULT", 0.30f,
		ready ? XMFLOAT3{ 1.0f, 1.0f, 1.0f } : XMFLOAT3{ 0.65f, 0.67f, 0.74f });

	DrawRect(bar_x - 3.0f, ULT_Y - 3.0f, bar_w + 6.0f, ULT_BAR_H + 6.0f, { 0.06f, 0.07f, 0.09f }, 0.90f);
	DrawRect(bar_x, ULT_Y, bar_w, ULT_BAR_H, { 0.14f, 0.16f, 0.22f }, 1.0f);

	float alpha = 1.0f;
	if (ready) { alpha = 0.65f + 0.35f * sinf(g_PulseTime * 8.0f); }
	DrawRect(bar_x, ULT_Y, bar_w * ult, ULT_BAR_H, col, alpha);
}

// --- item buff strip ---
static void DrawBuffStrip()
{
	if (!GameItem_IsBuffActive()) { return; }

	const int   icon = GameItem_GetBuffIconTexture();
	const float remain = GameItem_GetBuffRemaining();
	const float dur = GameItem_GetBuffDuration();
	const float pct = (dur > 0.0f) ? (remain / dur) : 0.0f;

	if (icon >= 0)
	{
		Sprite_Draw(icon, HP_X, BUFF_Y, BUFF_H, BUFF_H);
	}

	const float bar_x = HP_X + GUTTER;
	const float bar_w = HP_W - GUTTER;
	const float by = BUFF_Y + BUFF_H * 0.30f;
	const float bh = BUFF_H * 0.40f;

	DrawRect(bar_x - 3.0f, by - 3.0f, bar_w + 6.0f, bh + 6.0f, { 0.06f, 0.07f, 0.09f }, 0.90f);
	DrawRect(bar_x, by, bar_w, bh, { 0.14f, 0.16f, 0.22f }, 1.0f);
	DrawRect(bar_x, by, bar_w * pct, bh, { 0.95f, 0.82f, 0.25f }, 1.0f);

	char t[16];
	snprintf(t, sizeof(t), "%.0fs", ceilf(remain));
	GameText_Draw(bar_x + bar_w - 34.0f, BUFF_Y + 1.0f, t, 0.30f, { 0.90f, 0.90f, 0.95f });
}

static void DrawProgress()
{
	float y = PR_Y;

	// --- level + XP gauge ---
	char lv[32];
	snprintf(lv, sizeof(lv), "LV %d", GameProgress_GetLevel());
	GameText_Draw(PR_X, y, lv, 0.50f, { 1.0f, 1.0f, 1.0f });
	y += 34.0f;

	// fill from 0 each level: (xp - level start) / (next - level start)
	const int xp = GameProgress_GetXP();
	const int start = GameProgress_GetXPForCurrentLevel();
	const int next = GameProgress_GetXPForNextLevel();

	float xp_fraction = 1.0f;
	if (next > start)
	{
		xp_fraction = std::clamp(static_cast<float>(xp - start)
			/ static_cast<float>(next - start), 0.0f, 1.0f);
	}

	DrawRect(PR_X - 3.0f, y - 3.0f, PR_W + 6.0f, XP_H + 6.0f, { 0.06f, 0.07f, 0.09f }, 0.90f);
	DrawRect(PR_X, y, PR_W, XP_H, { 0.14f, 0.16f, 0.22f }, 1.0f);
	DrawRect(PR_X, y, PR_W * xp_fraction, XP_H, { 0.45f, 0.85f, 1.00f }, 1.0f);
	y += XP_H + 18.0f;

	// --- element levels ---
	for (int e = 0; e < ELEMENT_TYPE_COUNT; e++)
	{
		const int level = GameProgress_GetElementLevel(static_cast<ElementType>(e));
		const XMFLOAT3 col = ElementColor(e);

		GameText_Draw(PR_X, y - 2.0f, ElementShortName(e), 0.34f, col);

		const float pips_x = PR_X + 96.0f;
		const float pip_w = (PR_W - 96.0f - PIP_GAP * (ELEMENT_LEVEL_MAX - 1))
			/ ELEMENT_LEVEL_MAX;
		for (int i = 0; i < ELEMENT_LEVEL_MAX; i++)
		{
			const float x = pips_x + i * (pip_w + PIP_GAP);
			const bool  filled = (i < level);
			DrawRect(x, y, pip_w, PIP_H,
				filled ? col : XMFLOAT3{ 0.18f, 0.19f, 0.24f },
				filled ? 1.0f : 0.85f);
		}
		y += PIP_H + 8.0f;
	}
}

void GameUI_Draw()
{
	Sprite_SetFilter(kSpriteFilter_Linear);

	if (g_FlashTimer > 0.0f && g_FlashMax > 0.0f)
	{
		const float t = g_FlashTimer / g_FlashMax;
		DrawRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT,
			g_FlashColor, t * FLASH_MAX_ALPHA);
	}

	DrawBuffStrip();
	DrawUltimateBar();
	DrawHealthBar();
	DrawProgress();

	Sprite_SetFilter(kSpriteFilter_Point);
}

void GameUI_DrawScreenRect(float x, float y, float w, float h, const XMFLOAT3& color, float alpha)
{
	DrawRect(x, y, w, h, color, alpha);
}