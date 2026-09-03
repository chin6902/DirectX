/*============================================================================
Contents   :  [player_charge.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/16
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>

#include "player_charge.h"
#include "game_player.h"
#include "game_progress.h"
#include "game_skill.h"
#include "input_keyboard.h"
#include "input_mouse.h"
#include "camera.h"
#include "draw_primitives.h"
#include "game_enemy.h"
#include "game_item.h"
#include "game_audio.h"

using namespace DirectX;

// --- charge state ---
static ElementType g_Slots[CHARGE_SLOT_MAX]{};
static int         g_SlotCount = 0;
static int         g_Charging = ELEMENT_NONE;
static float       g_ChargeTimer = 0.0f;

static constexpr float CHARGE_TIME_BASE = 1.1f;

// --- ultimate meter ---
static constexpr int   ULTIMATE_SLOTS_NEEDED = 9;
static constexpr float ULTIMATE_QUEUE_DELAY = 0.45f;

static int         g_UltimateSpent[ELEMENT_TYPE_COUNT]{};
static int         g_UltimateTotal = 0;
static bool        g_UltimateQueued = false;
static float       g_UltimateDelay = 0.0f;
static ElementType g_UltimatePending = ELEMENT_FIRE;

// --- draw ---
static constexpr float SLOT_RADIUS = 9.0f;
static constexpr float SLOT_SPACING = 26.0f;
static constexpr float SLOT_HEIGHT = 60.0f;    
static constexpr float RING_MIN = 12.0f;
static constexpr float RING_GROWTH = 44.0f;

static void PlayChargeSound();

static int GetHeldElement()
{
	if (InputMouse_IsPress(MOUSE_BUTTON_LEFT)) { return ELEMENT_FIRE; }
	if (InputMouse_IsPress(MOUSE_BUTTON_MIDDLE)) { return ELEMENT_ICE; }
	if (InputMouse_IsPress(MOUSE_BUTTON_RIGHT)) { return ELEMENT_THUNDER; }
	return ELEMENT_NONE;
}

static int CountInSlots(int element)
{
	int n = 0;
	for (int i = 0; i < g_SlotCount; i++)
	{
		if (g_Slots[i] == element) { n++; }
	}
	return n;
}

static bool CanCharge(int element)
{
	if (element == ELEMENT_NONE) { return false; }
	if (g_SlotCount >= GameProgress_GetSlotCount()) { return false; }

	const int level = GameProgress_GetElementLevel(static_cast<ElementType>(element));
	return CountInSlots(element) < level;
}

void PlayerCharge_Initialize()
{
	g_SlotCount = 0;
	g_Charging = ELEMENT_NONE;
	g_ChargeTimer = 0.0f;

	g_UltimateTotal = 0;
	g_UltimateQueued = false;
	g_UltimateDelay = 0.0f;
	for (int e = 0; e < ELEMENT_TYPE_COUNT; e++) { g_UltimateSpent[e] = 0; }
}

void PlayerCharge_Finalize() 
{

}

bool PlayerCharge_IsChargeKeyHeld() { return CanCharge(GetHeldElement()); }
int  PlayerCharge_GetSlotCount() { return g_SlotCount; }

void PlayerCharge_Update(float delta_time)
{
	const int held = GetHeldElement();

	if (!CanCharge(held))
	{
		g_Charging = ELEMENT_NONE;
		g_ChargeTimer = 0.0f;      
		return;
	}

	if (held != g_Charging)       
	{
		g_Charging = held;
		g_ChargeTimer = 0.0f;
		PlayChargeSound();
	}

	const float charge_time = CHARGE_TIME_BASE * GameProgress_GetChargeTimeMul() * GameItem_GetChargeTimeMul();

	g_ChargeTimer += delta_time;
	if (g_ChargeTimer >= charge_time)
	{
		g_Slots[g_SlotCount++] = static_cast<ElementType>(g_Charging);
		g_ChargeTimer -= charge_time;

		if (CanCharge(g_Charging)) { PlayChargeSound(); }
	}
}

void PlayerCharge_UpdateAlways(float delta_time)
{
	if (!CanCharge(GetHeldElement()))
	{
		g_Charging = ELEMENT_NONE;
		g_ChargeTimer = 0.0f;
	}

	if (!g_UltimateQueued) { return; }

	g_UltimateDelay -= delta_time;
	if (g_UltimateDelay > 0.0f) { return; }

	GameSkill_CastUltimate(
		g_UltimatePending,
		Vector2{ GamePlayer_GetPosX(), GamePlayer_GetPosY() },
		GamePlayer_GetAimDir());

	g_UltimateQueued = false;
}

static void FeedUltimateMeter(const int* counts)
{
	for (int e = 0; e < ELEMENT_TYPE_COUNT; e++)
	{
		if (counts[e] == 0) { continue; }

		if (GameProgress_GetElementLevel(static_cast<ElementType>(e)) < 2) { continue; }

		g_UltimateSpent[e] += counts[e];
		g_UltimateTotal += counts[e];
	}

	if (g_UltimateTotal < ULTIMATE_SLOTS_NEEDED) { return; }

	g_UltimateQueued = true;
	g_UltimatePending = PlayerCharge_GetUltimateElement();
	g_UltimateDelay = ULTIMATE_QUEUE_DELAY;

	g_UltimateTotal = 0;
	for (int e = 0; e < ELEMENT_TYPE_COUNT; e++) { g_UltimateSpent[e] = 0; }
}

bool PlayerCharge_TryRelease()
{
	if (g_SlotCount == 0) { return false; }

	int counts[ELEMENT_TYPE_COUNT]{};
	for (int i = 0; i < g_SlotCount; i++) { counts[g_Slots[i]]++; }

	const SkillId id = GameSkill_Lookup(counts);

	int power = 0;
	for (int e = 0; e < ELEMENT_TYPE_COUNT; e++)
	{
		power += counts[e] * GameProgress_GetElementLevel(static_cast<ElementType>(e));
	}

	unsigned int element_mask = 0;
	for (int e = 0; e < ELEMENT_TYPE_COUNT; e++)
	{
		if (counts[e] > 0) { element_mask |= ElementBit(static_cast<ElementType>(e)); }
	}


	GameSkill_Cast(id,
		Vector2{ GamePlayer_GetPosX(), GamePlayer_GetPosY() },
		GamePlayer_GetAimDir(),
		std::max(1, power),
		element_mask);

	FeedUltimateMeter(counts);

	g_SlotCount = 0;
	g_Charging = ELEMENT_NONE;
	g_ChargeTimer = 0.0f;
	return true;
}

float PlayerCharge_GetUltimateFraction()
{
	return std::min(1.0f, static_cast<float>(g_UltimateTotal) / static_cast<float>(ULTIMATE_SLOTS_NEEDED));
}

ElementType PlayerCharge_GetUltimateElement()
{
	ElementType best = ELEMENT_FIRE;
	int best_score = -1;

	for (int e = 0; e < ELEMENT_TYPE_COUNT; e++)
	{
		const int score = g_UltimateSpent[e]
			* GameProgress_GetElementLevel(static_cast<ElementType>(e));
		if (score > best_score)
		{
			best_score = score;
			best = static_cast<ElementType>(e);
		}
	}
	return best;
}

static void PlayChargeSound()
{
	const float charge_mul = GameProgress_GetChargeTimeMul() * GameItem_GetChargeTimeMul();
	GameAudio_PlayPitched(SND_CHARGE, 1.0f / std::max(0.25f, charge_mul));
}

// ============================================================================
// Draw
// ============================================================================
static XMFLOAT3 ElementColor(int e)
{
	switch (e)
	{
	case ELEMENT_FIRE:    return { 1.00f, 0.35f, 0.10f };
	case ELEMENT_ICE:     return { 0.30f, 0.70f, 1.00f };
	case ELEMENT_THUNDER: return { 1.00f, 0.90f, 0.20f };
	default:              return { 0.35f, 0.35f, 0.35f };
	}
}

void PlayerCharge_Draw()
{
	const Vector2 player{ GamePlayer_GetPosX(), GamePlayer_GetPosY() };
	const int max_slots = GameProgress_GetSlotCount();

	// --- slot ---
	for (int i = 0; i < max_slots; i++)
	{
		const Vector2 p{
			player.x + (i - (max_slots - 1) * 0.5f) * SLOT_SPACING,
			player.y - SLOT_HEIGHT
		};

		if (i < g_SlotCount)
		{
			DrawPrim_Ring(p, SLOT_RADIUS, 2.5f, ElementColor(g_Slots[i]), 0.95f);
		}
		else
		{
			DrawPrim_Ring(p, SLOT_RADIUS, 1.5f, { 0.45f, 0.45f, 0.50f }, 0.75f);
		}
	}

	// --- charge progress ---
	if (g_Charging != ELEMENT_NONE)
	{
		const float charge_time = CHARGE_TIME_BASE * GameProgress_GetChargeTimeMul() * GameItem_GetChargeTimeMul();
		const float t = std::min(g_ChargeTimer / charge_time, 1.0f);
		const XMFLOAT3 col = ElementColor(g_Charging);

		DrawPrim_Ring(player, RING_MIN + RING_GROWTH * t, 3.0f, col, 0.85f);
	}
}