/*============================================================================
Contents   :  [game_player.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/24
-----------------------------------------------------------------------------

============================================================================*/
#include <DirectXMath.h>	

#include "game_player.h"
#include "texture.h"
#include "sprite.h" //"flipbook animation"
#include "input_keyboard.h"
#include "input_mouse.h"
#include "config.h"
#include "game_playerBullet.h"
#include "vector2.h"
#include "camera.h"
#include "game_stage.h"
#include "collision_debug.h"

#include "player_attack.h"
#include "player_charge.h"

using namespace DirectX;

//player texture ID
static int g_player_texture_ID = -1;
static float g_player_width = 96.0f;
static float g_player_height = 64.0f;
static constexpr float PLAYER_COLLIDER_RADIUS = 25.6f;
		
//player position
static Vector2 g_Pos;
static float g_Speed = 300.0f;

static Vector2 g_AimDir{ 1.0f, 0.0f };

static void UpdateAim();
static void UpdateMovement(float delta_time);

enum PlayerState
{
	PLAYER_STATE_NORMAL,
	PLAYER_STATE_CHARGING,
};

static PlayerState g_State = PLAYER_STATE_NORMAL;

static void ChangeState(PlayerState nextState)
{
	if (g_State == nextState) { return; }       

	g_State = nextState;
}

void GamePlayer_Initialize(float startX, float startY)
{				
	//プレイヤーのテクスチャを読み込む
	g_player_texture_ID = Texture_Load(L"assets/textures/player.png");
	g_Pos = { startX, startY };

	PlayerAttack_Initialize();
	PlayerCharge_Initialize();
}

void GamePlayer_Finalize()
{
	PlayerAttack_Finalize();
	PlayerCharge_Finalize();
	//プレイヤーのテクスチャを解放する
	Texture_Release(g_player_texture_ID);
}

void GamePlayer_Update(float delta_time)
{
	UpdateAim();
	UpdateMovement(delta_time);

	if (InputKeyboard_IsTrigger(KK_SPACE))
	{
		PlayerCharge_TryRelease();
	}

	switch (g_State)
	{
	case PLAYER_STATE_NORMAL:
		if (PlayerCharge_IsChargeKeyHeld()) { ChangeState(PLAYER_STATE_CHARGING); break; }
		PlayerAttack_Update(delta_time);
		break;

	case PLAYER_STATE_CHARGING:
		PlayerCharge_Update(delta_time);
		if (!PlayerCharge_IsChargeKeyHeld()) { ChangeState(PLAYER_STATE_NORMAL); }
		break;
	}
}

void GamePlayer_Draw()
{
	//座標にプレイヤーを描画する
	Sprite_Draw(
		g_player_texture_ID, 
		Camera_WorldToScreenX(g_Pos.x - g_player_width * 0.5f), Camera_WorldToScreenY(g_Pos.y - g_player_height * 0.5f), 
		g_player_width, g_player_height
	);

	PlayerAttack_Draw();
	PlayerCharge_Draw();

#ifdef _DEBUG
	Collision_Debug_Draw(
		{ { Camera_WorldToScreenX(g_Pos.x), Camera_WorldToScreenY(g_Pos.y) }, PLAYER_COLLIDER_RADIUS },
		{ 0.0f, 1.0f, 0.0f });

	const Vector2 marker = g_Pos + g_AimDir * 100.0f;
	Collision_Debug_Draw(
		{ { Camera_WorldToScreenX(marker.x), Camera_WorldToScreenY(marker.y) }, 6.0f },
		{ 1.0f, 1.0f, 0.0f });
#endif
}

static void UpdateMovement(float delta_time)
{
	Vector2 dir{ 0.0f, 0.0f };

	if (InputKeyboard_IsPress(KK_W)) { dir.y -= 1.0f; }
	if (InputKeyboard_IsPress(KK_S)) { dir.y += 1.0f; }
	if (InputKeyboard_IsPress(KK_A)) { dir.x -= 1.0f; }
	if (InputKeyboard_IsPress(KK_D)) { dir.x += 1.0f; }

	if (dir.IsZero())
	{
		return;
	}

	const Vector2 old_pos = g_Pos;
	g_Pos += Vector2_Normalize(dir) * (g_Speed * delta_time);
	g_Pos = GameStage_ResolvePosition(old_pos, g_Pos, PLAYER_COLLIDER_RADIUS);
}

static void UpdateAim()
{
	const Vector2 mouse_world{
		Camera_ScreenToWorldX(static_cast<float>(InputMouse_GetX())),
		Camera_ScreenToWorldY(static_cast<float>(InputMouse_GetY()))
	};

	const Vector2 to_mouse = mouse_world - g_Pos;
	if (to_mouse.LengthSq() > 1.0f)      // cursor on top of player -> keep last aim
	{
		g_AimDir = Vector2_Normalize(to_mouse);
	}
}

float GamePlayer_GetPosX()
{
	return g_Pos.x;
}

float GamePlayer_GetPosY()
{
	return g_Pos.y;
}

float GamePlayer_GetSpeed()
{
	return g_Speed;
}

Vector2 GamePlayer_GetAimDir()
{
	return g_AimDir;
}

void GamePlayer_SetSpeed(float speed)
{
	if (speed < 0.0f)
	{
		speed = 0.0f;
	}

	g_Speed = speed;
}
