/*============================================================================
Contents   :  [game_item.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/24
-----------------------------------------------------------------------------

============================================================================*/
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "game_item.h"
#include "game_progress.h"
#include "game_player.h"
#include "game_stage.h"
#include "camera.h"
#include "texture.h"
#include "sprite.h"
#include "collision.h"
#include "game_audio.h"

static constexpr float BUFF_DURATION = 15.0f;
static constexpr float FIRE_DAMAGE_MUL = 1.3f;   
static constexpr float STORM_CHARGE_MUL = 0.8f;   
static constexpr float POTION_DROP_CHANCE = 0.05f;  
static constexpr float POTION_HEAL = 2.0f;
static constexpr float STAFF_RESPAWN_DELAY = 20.0f; 

static constexpr float STAFF_DRAW_W = 56.0f;
static constexpr float STAFF_DRAW_H = 56.0f;
static constexpr float POTION_DRAW = 32.0f;

static constexpr float ITEM_COLLECT_RADIUS = 22.0f;
static constexpr float ITEM_ACCEL = 1500.0f;
static constexpr float ITEM_SPEED_MAX = 900.0f;
static constexpr float ITEM_BOB_RATE = 3.0f;
static constexpr float ITEM_BOB_AMOUNT = 3.0f;

static constexpr int   POTION_MAX = 64;

static int g_tex_fire_staff = -1;
static int g_tex_storm_staff = -1;
static int g_tex_potion = -1;

struct StaffPickup
{
	Vector2 pos{ 0.0f, 0.0f };
	float   speed = 0.0f;
	float   bob_phase = 0.0f;
	bool    attracted = false;
	bool    active = false;
};
static StaffPickup g_Staff;
static float       g_StaffRespawnTimer = 0.0f;

struct Potion
{
	Vector2 pos{ 0.0f, 0.0f };
	float   speed = 0.0f;
	float   bob_phase = 0.0f;
	bool    attracted = false;
	bool    active = false;
};
static Potion g_Potions[POTION_MAX]{};

static float g_DamageMul = 1.0f;
static float g_ChargeMul = 1.0f;
static float g_BuffTimer = 0.0f;
static float g_BuffDuration = BUFF_DURATION;
static int   g_BuffIconTex = -1;

static std::mt19937& Rng()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	return gen;
}

static ItemType ChosenItem() { return GameProgress_GetChosenItem(); }

static bool ChosenIsStaff()
{
	const ItemType it = ChosenItem();
	return it == ITEM_FIRE_STAFF || it == ITEM_ELECTRIC_STAFF;
}

static int StaffTexture()
{
	return (ChosenItem() == ITEM_FIRE_STAFF) ? g_tex_fire_staff : g_tex_storm_staff;
}

static void ApplyStaffBuff()
{
	if (ChosenItem() == ITEM_FIRE_STAFF)
	{
		g_DamageMul = FIRE_DAMAGE_MUL;
		g_ChargeMul = 1.0f;
	}
	else // electric / storm
	{
		g_ChargeMul = STORM_CHARGE_MUL;
		g_DamageMul = 1.0f;
	}
	g_BuffTimer = BUFF_DURATION;  
	g_BuffDuration = BUFF_DURATION;
	g_BuffIconTex = StaffTexture();
}

void GameItem_Initialize()
{
	g_tex_fire_staff = Texture_Load(L"assets/textures/fire_staff.png");
	g_tex_storm_staff = Texture_Load(L"assets/textures/electric_staff.png");
	g_tex_potion = Texture_Load(L"assets/textures/potion.png");

	g_Staff = {};
	g_StaffRespawnTimer = 0.0f;  
	for (Potion& p : g_Potions) { p = {}; }

	g_DamageMul = 1.0f;
	g_ChargeMul = 1.0f;
	g_BuffTimer = 0.0f;
	g_BuffIconTex = -1;
}

void GameItem_Finalize()
{
	Texture_Release(g_tex_fire_staff);
	Texture_Release(g_tex_storm_staff);
	Texture_Release(g_tex_potion);
}

static bool AttractToPlayer(Vector2& pos, float& speed, bool& attracted,
	float& bob_phase, float dt)
{
	const Vector2 player{ GamePlayer_GetPosX(), GamePlayer_GetPosY() };
	const Vector2 to_player = player - pos;
	const float dist_sq = to_player.LengthSq();

	if (dist_sq < ITEM_COLLECT_RADIUS * ITEM_COLLECT_RADIUS) { return true; }

	const float pickup = GameProgress_GetPickUpRange();
	if (!attracted && dist_sq < pickup * pickup) { attracted = true; }

	if (attracted)
	{
		speed = std::min(speed + ITEM_ACCEL * dt, ITEM_SPEED_MAX);
		pos += Vector2_Normalize(to_player) * (speed * dt);
	}
	else
	{
		bob_phase += ITEM_BOB_RATE * dt;
	}
	return false;
}

static void UpdateStaff(float dt)
{
	if (!ChosenIsStaff()) { return; }

	if (!g_Staff.active)
	{
		g_StaffRespawnTimer -= dt;
		if (g_StaffRespawnTimer <= 0.0f)
		{
			float sx, sy;
			if (GameStage_TryGetObstacleSpawnPoint(sx, sy))  
			{
				g_Staff = {};
				g_Staff.pos = { sx, sy };
				g_Staff.bob_phase = 0.0f;
				g_Staff.active = true;
			}
		}
		return;
	}

	if (AttractToPlayer(g_Staff.pos, g_Staff.speed, g_Staff.attracted,
		g_Staff.bob_phase, dt))
	{
		ApplyStaffBuff();
		GameAudio_PlayPitched(SND_PICKUP, 0.70f);
		g_Staff.active = false;
		g_StaffRespawnTimer = STAFF_RESPAWN_DELAY;
	}
}

static void UpdatePotions(float dt)
{
	for (Potion& p : g_Potions)
	{
		if (!p.active) { continue; }

		if (AttractToPlayer(p.pos, p.speed, p.attracted, p.bob_phase, dt))
		{
			GamePlayer_Heal(POTION_HEAL);
			GameAudio_PlayPitched(SND_PICKUP, 0.85f);
			p.active = false;
		}
	}
}

static void UpdateBuff(float dt)
{
	if (g_BuffTimer <= 0.0f) { return; }

	g_BuffTimer -= dt;
	if (g_BuffTimer <= 0.0f)
	{
		g_BuffTimer = 0.0f;
		g_DamageMul = 1.0f;
		g_ChargeMul = 1.0f;
		g_BuffIconTex = -1;
	}
}

void GameItem_Update(float dt)
{
	UpdateStaff(dt);
	UpdatePotions(dt);
	UpdateBuff(dt);
}

void GameItem_OnEnemyKilled(const Vector2& pos)
{
	if (ChosenItem() != ITEM_POTION) { return; }

	std::uniform_real_distribution<float> roll(0.0f, 1.0f);
	if (roll(Rng()) >= POTION_DROP_CHANCE) { return; }

	for (Potion& p : g_Potions)
	{
		if (!p.active)
		{
			p = {};
			p.pos = pos;
			p.bob_phase = (rand() % 628) * 0.01f;   
			p.active = true;
			return;
		}
	}
}

float GameItem_GetDamageMul() { return g_DamageMul; }
float GameItem_GetChargeTimeMul() { return g_ChargeMul; }

bool  GameItem_IsBuffActive() { return g_BuffTimer > 0.0f; }
float GameItem_GetBuffRemaining() { return g_BuffTimer; }
float GameItem_GetBuffDuration() { return g_BuffDuration; }
int   GameItem_GetBuffIconTexture() { return g_BuffIconTex; }

int GameItem_GetItemTexture(ItemType item)
{
	switch (item)
	{
	case ITEM_FIRE_STAFF:     return g_tex_fire_staff;
	case ITEM_ELECTRIC_STAFF: return g_tex_storm_staff;
	case ITEM_POTION:         return g_tex_potion;
	default:                  return -1;
	}
}

void GameItem_Draw()
{
	if (ChosenIsStaff() && g_Staff.active)
	{
		const float bob = g_Staff.attracted
			? 0.0f : sinf(g_Staff.bob_phase) * ITEM_BOB_AMOUNT;
		const float sx = Camera_WorldToScreenX(g_Staff.pos.x);
		const float sy = Camera_WorldToScreenY(g_Staff.pos.y + bob);
		Sprite_Draw(StaffTexture(),
			sx - STAFF_DRAW_W * 0.5f, sy - STAFF_DRAW_H * 0.5f,
			STAFF_DRAW_W, STAFF_DRAW_H);
	}

	for (const Potion& p : g_Potions)
	{
		if (!p.active) { continue; }
		const float bob = p.attracted ? 0.0f : sinf(p.bob_phase) * ITEM_BOB_AMOUNT;
		const float sx = Camera_WorldToScreenX(p.pos.x);
		const float sy = Camera_WorldToScreenY(p.pos.y + bob);
		Sprite_Draw(g_tex_potion,
			sx - POTION_DRAW * 0.5f, sy - POTION_DRAW * 0.5f,
			POTION_DRAW, POTION_DRAW);
	}
}