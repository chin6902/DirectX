/*============================================================================
Contents   :  [game_player.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/15
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <DirectXMath.h>

#include "game_player.h"
#include "health.h"
#include "texture.h"
#include "sprite.h"
#include "input_keyboard.h"
#include "input_mouse.h"
#include "config.h"
#include "vector2.h"
#include "camera.h"
#include "game_stage.h"
#include "game_progress.h"
#include "collision_debug.h"
#include "draw_primitives.h"

#include "player_attack.h"
#include "player_charge.h"

using namespace DirectX;


static constexpr float PLAYER_WIDTH = 96.0f;
static constexpr float PLAYER_HEIGHT = 64.0f;
static constexpr float PLAYER_COLLIDER_RADIUS = 25.6f;

static constexpr float CHARGE_SPEED_MUL = 0.45f;   
static constexpr float KNOCKBACK_FRICTION = 0.86f;  
static constexpr float HIT_FLASH_TIME = 0.12f;


static int     g_player_texture_ID = -1;

static Vector2 g_Pos;
static Vector2 g_AimDir{ 1.0f, 0.0f };

// --- health ---
static Health  g_Health;
static float   g_InvincTimer = 0.0f;
static float   g_FlashTimer = 0.0f;

// --- shield
static float   g_ShieldTimer = 0.0f;
static int     g_ShieldCharges = 0;

// --- hit reaction
static float   g_KnockTimer = 0.0f;
static Vector2 g_KnockVel{ 0.0f, 0.0f };

enum PlayerState
{
	PLAYER_STATE_NORMAL,
	PLAYER_STATE_CHARGING,
};

static PlayerState g_State = PLAYER_STATE_NORMAL;

static void UpdateAim();
static void SyncMaxHP();
static void UpdateRegen(float delta_time);
static void UpdateKnockback(float delta_time);
static void UpdateMovement(float delta_time);

static void ChangeState(PlayerState nextState)
{
	if (g_State == nextState) { return; }
	g_State = nextState;
}

void GamePlayer_Initialize(float startX, float startY)
{
	g_player_texture_ID = Texture_Load(L"assets/textures/player.png");

	g_Pos = { startX, startY };
	g_AimDir = { 1.0f, 0.0f };

	g_Health.Reset(GameProgress_GetStat(STAT_MAX_HP));

	g_InvincTimer = 0.0f;
	g_FlashTimer = 0.0f;
	g_KnockTimer = 0.0f;
	g_KnockVel = { 0.0f, 0.0f };
	g_ShieldTimer = 0.0f;
	g_ShieldCharges = 0;
	g_State = PLAYER_STATE_NORMAL;

	PlayerAttack_Initialize();
	PlayerCharge_Initialize();
}

void GamePlayer_Finalize()
{
	PlayerAttack_Finalize();
	PlayerCharge_Finalize();
	Texture_Release(g_player_texture_ID);
}

void GamePlayer_Update(float delta_time)
{
	UpdateAim();
	SyncMaxHP();

	if (g_FlashTimer > 0.0f) { g_FlashTimer -= delta_time; }
	if (g_InvincTimer > 0.0f) { g_InvincTimer -= delta_time; }

	if (g_ShieldTimer > 0.0f)
	{
		g_ShieldTimer -= delta_time;
		if (g_ShieldTimer <= 0.0f) { g_ShieldCharges = 0; }
	}

	UpdateRegen(delta_time);
	UpdateKnockback(delta_time);
	UpdateMovement(delta_time);

	PlayerCharge_UpdateAlways(delta_time);

	if (InputKeyboard_IsTrigger(KK_SPACE))
	{
		PlayerCharge_TryRelease();
	}

	switch (g_State)
	{
	case PLAYER_STATE_NORMAL:
		if (PlayerCharge_IsChargeKeyHeld())
		{
			ChangeState(PLAYER_STATE_CHARGING);
			break;
		}
		break;

	case PLAYER_STATE_CHARGING:
		PlayerCharge_Update(delta_time);
		if (!PlayerCharge_IsChargeKeyHeld()) { ChangeState(PLAYER_STATE_NORMAL); }
		break;
	}
}

static void SyncMaxHP()
{
	const float max_hp = GameProgress_GetStat(STAT_MAX_HP);
	if (max_hp != g_Health.max)
	{
		g_Health.SetMax(max_hp, true); 
	}
}

static void UpdateRegen(float delta_time)
{
	const float regen = GameProgress_GetStat(STAT_HP_REGEN);
	if (regen <= 0.0f) { return; }

	g_Health.Heal(regen * delta_time);  
}

static void UpdateKnockback(float delta_time)
{
	if (g_KnockTimer <= 0.0f) { return; }

	g_KnockTimer -= delta_time;

	const Vector2 old_pos = g_Pos;
	g_Pos += g_KnockVel * delta_time;
	g_Pos = GameStage_ResolvePosition(old_pos, g_Pos, PLAYER_COLLIDER_RADIUS);

	g_KnockVel *= KNOCKBACK_FRICTION;
}

static void UpdateMovement(float delta_time)
{
	Vector2 dir{ 0.0f, 0.0f };

	if (InputKeyboard_IsPress(KK_W)) { dir.y -= 1.0f; }
	if (InputKeyboard_IsPress(KK_S)) { dir.y += 1.0f; }
	if (InputKeyboard_IsPress(KK_A)) { dir.x -= 1.0f; }
	if (InputKeyboard_IsPress(KK_D)) { dir.x += 1.0f; }

	if (dir.IsZero()) { return; }

	const float speed_mul = (g_State == PLAYER_STATE_CHARGING) ? CHARGE_SPEED_MUL : 1.0f;
	const float speed = GameProgress_GetStat(STAT_MOVE_SPEED);

	const Vector2 old_pos = g_Pos;
	g_Pos += Vector2_Normalize(dir) * (speed * speed_mul * delta_time);
	g_Pos = GameStage_ResolvePosition(old_pos, g_Pos, PLAYER_COLLIDER_RADIUS);
}

static void UpdateAim()
{
	const Vector2 mouse_world{
		Camera_ScreenToWorldX(static_cast<float>(InputMouse_GetX())),
		Camera_ScreenToWorldY(static_cast<float>(InputMouse_GetY()))
	};

	const Vector2 to_mouse = mouse_world - g_Pos;
	if (to_mouse.LengthSq() > 1.0f)  
	{
		g_AimDir = Vector2_Normalize(to_mouse);
	}
}

// ============================================================================
// Health
// ============================================================================
void GamePlayer_TakeHit(const PlayerHit& hit)
{
	if (g_InvincTimer > 0.0f || g_Health.IsDead()) { return; }

	if (g_ShieldCharges > 0 && g_ShieldTimer > 0.0f)
	{
		g_ShieldCharges--;
		g_InvincTimer = GameProgress_GetStat(STAT_IFRAME_TIME);
		g_FlashTimer = HIT_FLASH_TIME;
		if (g_ShieldCharges <= 0) { g_ShieldTimer = 0.0f; }
		return;
	}

	const int reduction = static_cast<int>(GameProgress_GetStat(STAT_DAMAGE_REDUCTION));
	const int damage = std::max(1, hit.damage - reduction);

	g_Health.Damage(static_cast<float>(damage));

	g_InvincTimer = GameProgress_GetStat(STAT_IFRAME_TIME);
	g_FlashTimer = HIT_FLASH_TIME;

	if (hit.knockback_speed > 0.0f && hit.knockback_time > 0.0f)
	{
		g_KnockVel = hit.direction * hit.knockback_speed;
		g_KnockTimer = std::max(g_KnockTimer, hit.knockback_time);
	}
}

void GamePlayer_Heal(float amount) { g_Health.Heal(amount); }

void GamePlayer_GrantShield(float duration, int charges)
{
	g_ShieldTimer = std::max(g_ShieldTimer, duration);
	g_ShieldCharges = std::max(g_ShieldCharges, charges);
}

int  GamePlayer_GetShieldCharges() { return g_ShieldCharges; }
bool GamePlayer_IsShielded() { return g_ShieldCharges > 0 && g_ShieldTimer > 0.0f; }

int   GamePlayer_GetHP() { return g_Health.Display(); }
int   GamePlayer_GetMaxHP() { return static_cast<int>(g_Health.max); }
float GamePlayer_GetHPFraction() { return g_Health.Fraction(); }
bool  GamePlayer_IsDead() { return g_Health.IsDead(); }
bool  GamePlayer_IsInvincible() { return g_InvincTimer > 0.0f; }

// ============================================================================
// Draw
// ============================================================================
void GamePlayer_Draw()
{
	SpriteDrawParams p;
	p.flip_x = (g_AimDir.x < 0.0f);

	// hit flash
	if (g_FlashTimer > 0.0f)
	{
		p.color = { 2.5f, 2.5f, 2.5f };
	}
	else if (g_InvincTimer > 0.0f)
	{
		p.alpha = (fmodf(g_InvincTimer, 0.16f) < 0.08f) ? 0.35f : 1.0f;
	}

	Sprite_Draw(
		g_player_texture_ID,
		Camera_WorldToScreenX(g_Pos.x - PLAYER_WIDTH * 0.5f),
		Camera_WorldToScreenY(g_Pos.y - PLAYER_HEIGHT * 0.5f),
		PLAYER_WIDTH, PLAYER_HEIGHT,
		p);

	// shield ring
	if (GamePlayer_IsShielded())
	{
		const float fade = std::min(g_ShieldTimer, 1.0f);
		DrawPrim_Ring(g_Pos, 46.0f, 3.0f, { 0.75f, 0.90f, 1.0f }, 0.85f * fade);
		DrawPrim_Ring(g_Pos, 40.0f, 1.5f, { 1.0f, 1.0f, 1.0f }, 0.45f * fade);
	}

	PlayerAttack_Draw();
	PlayerCharge_Draw();

#ifdef _DEBUG
	Collision_Debug_Draw(
		{ { Camera_WorldToScreenX(g_Pos.x), Camera_WorldToScreenY(g_Pos.y) },
		  PLAYER_COLLIDER_RADIUS },
		(g_InvincTimer > 0.0f) ? XMFLOAT3{ 1.0f, 1.0f, 0.0f }
	: XMFLOAT3{ 0.0f, 1.0f, 0.0f });

#endif
}

float   GamePlayer_GetPosX() { return g_Pos.x; }
float   GamePlayer_GetPosY() { return g_Pos.y; }
Vector2 GamePlayer_GetPos() { return g_Pos; }
Vector2 GamePlayer_GetAimDir() { return g_AimDir; }
float   GamePlayer_GetSpeed() { return GameProgress_GetStat(STAT_MOVE_SPEED); }

CollisionCircle GamePlayer_GetCollisionCircle()
{
	return { { g_Pos.x, g_Pos.y }, PLAYER_COLLIDER_RADIUS };
}