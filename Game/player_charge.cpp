/*============================================================================
Contents   :  [player_charge.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/05
-----------------------------------------------------------------------------

============================================================================*/
#include "player_charge.h"
#include "input_keyboard.h"
#include "input_mouse.h"
#include "collision_debug.h"
#include "vector2.h"
#include "game_player.h"
#include "camera.h"
#include "game_skill.h"

static ElementType g_Slots[CHARGE_SLOT_MAX]{};
static int g_SlotCount = 0;
static int g_Charging = ELEMENT_NONE;
static float g_ChargeTimer = 0.0f;

static constexpr float CHARGE_TIME_BASE = 1.5f;
static float g_ChargeSpeedMul = 1.0f;

static int GetHeldElement()                     // input binding lives in ONE place
{
	if (InputMouse_IsPress(MOUSE_BUTTON_LEFT)) { return ELEMENT_FIRE; }
	if (InputMouse_IsPress(MOUSE_BUTTON_MIDDLE)) { return ELEMENT_ICE; }
	if (InputMouse_IsPress(MOUSE_BUTTON_RIGHT)) { return ELEMENT_THUNDER; }
	return ELEMENT_NONE;
}

void PlayerCharge_Initialize()
{
	g_SlotCount = 0;
	g_Charging = ELEMENT_NONE;
	g_ChargeTimer = 0.0f;
}

void PlayerCharge_Finalize()
{

}

void PlayerCharge_Update(float delta_time)
{
	const int held = GetHeldElement();

	if (held == ELEMENT_NONE || g_SlotCount >= CHARGE_SLOT_MAX)
	{
		g_Charging = ELEMENT_NONE;
		g_ChargeTimer = 0.0f;
		return;
	}

	if (held != g_Charging)
	{
		g_Charging = held;
		g_ChargeTimer = 0.0f;
	}

	g_ChargeTimer += delta_time * g_ChargeSpeedMul;

	if (g_ChargeTimer >= CHARGE_TIME_BASE)
	{
		g_Slots[g_SlotCount++] = static_cast<ElementType>(g_Charging);
		g_ChargeTimer -= CHARGE_TIME_BASE;
	}
}

void PlayerCharge_Draw()
{
	const Vector2 base = {
		GamePlayer_GetPosX(),
		GamePlayer_GetPosY() - 60.0f          // above the sprite
	};

	// --- three slot ---
	for (int i = 0; i < CHARGE_SLOT_MAX; i++)
	{
		const float x = base.x + (i - 1) * 26.0f;   // -26, 0, +26

		DirectX::XMFLOAT3 color{ 0.35f, 0.35f, 0.35f };   // empty
		if (i < g_SlotCount)
		{
			switch (g_Slots[i])
			{
			case ELEMENT_FIRE:    color = { 1.0f, 0.35f, 0.1f }; break;
			case ELEMENT_ICE:     color = { 0.3f, 0.7f, 1.0f };  break;
			case ELEMENT_THUNDER: color = { 1.0f, 0.9f, 0.2f };  break;
			}
		}

		Collision_Debug_Draw(
			{ { Camera_WorldToScreenX(x), Camera_WorldToScreenY(base.y) }, 10.0f },
			color);
	}

	// --- charge progress: a circle that grows as the current slot fills ---
	if (g_Charging != ELEMENT_NONE)
	{
		const float t = g_ChargeTimer / CHARGE_TIME_BASE;      // 0 -> 1
		DirectX::XMFLOAT3 color{ 1.0f, 1.0f, 1.0f };
		switch (g_Charging)
		{
		case ELEMENT_FIRE:    color = { 1.0f, 0.35f, 0.1f }; break;
		case ELEMENT_ICE:     color = { 0.3f, 0.7f, 1.0f };  break;
		case ELEMENT_THUNDER: color = { 1.0f, 0.9f, 0.2f };  break;
		}

		Collision_Debug_Draw(
			{ { Camera_WorldToScreenX(GamePlayer_GetPosX()),
				Camera_WorldToScreenY(GamePlayer_GetPosY()) },
			  10.0f + t * 45.0f },                            // grows toward completion
			color);
	}
}

bool PlayerCharge_IsChargeKeyHeld()
{
	return GetHeldElement() != ELEMENT_NONE && g_SlotCount < CHARGE_SLOT_MAX;
}

bool PlayerCharge_TryRelease()
{
	if (g_SlotCount == 0)
	{
		return false;                 
	}

	// --- Slots -> multiset counts (this is what makes order irrelevant) ---
	int counts[ELEMENT_TYPE_COUNT]{};      // zero-initialised: {0,0,0}
	for (int i = 0; i < g_SlotCount; i++)
	{
		counts[g_Slots[i]]++;
	}

	// --- Counts -> recipe ---
	const SkillId id = GameSkill_Lookup(counts);

	// --- Power: for now just the number of slots spent.
	//     Later: sum of counts[e] * element_level[e]. ---
	int power = 0;
	for (int e = 0; e < ELEMENT_TYPE_COUNT; e++)
	{
		power += counts[e];
	}

	GameSkill_Cast(
		id,
		Vector2{ GamePlayer_GetPosX(), GamePlayer_GetPosY() },
		GamePlayer_GetAimDir(),
		power);

	// --- Release always empties the hand (your chosen semantics) ---
	g_SlotCount = 0;
	g_Charging = ELEMENT_NONE;
	g_ChargeTimer = 0.0f;
	return true;
}

int PlayerCharge_GetSlotCount()
{
	return g_SlotCount;
}
