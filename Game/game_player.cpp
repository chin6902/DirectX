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
#include "config.h"
#include "game_playerBullet.h"

using namespace DirectX;

//player texture ID
static int g_player_texture_ID = -1;
static float g_player_width = 96.0f;
static float g_player_height = 64.0f;
		
//player position
static float g_playerX = 0.0f;
static float g_playerY = 0.0f;
static float g_playerOffsetX = 0.0f;
static float g_playerOffsetY = 0.0f;
static float g_playerSpeed = 200.0f;

void GamePlayer_Initialize(float startX, float startY)
{				
	//プレイヤーのテクスチャを読み込む
	g_player_texture_ID = Texture_Load(L"assets/textures/Player - Copy.png");
	g_playerX = startX;
	g_playerY = startY;
}

void GamePlayer_Finalize()
{
	//プレイヤーのテクスチャを解放する
	Texture_Release(g_player_texture_ID);
}

void GamePlayer_Update(float delta_time)
{
	DirectX::XMFLOAT2 velocity = { 0.0f, 0.0f };

	if (InputKeyboard_IsPress(KK_A) || InputKeyboard_IsPress(KK_LEFT))
	{
		velocity.x -= 1.0f;
	}
	if (InputKeyboard_IsPress(KK_D) || InputKeyboard_IsPress(KK_RIGHT))
	{
		velocity.x += 1.0f;
	}
	if (InputKeyboard_IsPress(KK_W) || InputKeyboard_IsPress(KK_UP))
	{
		velocity.y -= 1.0f;
	}
	if (InputKeyboard_IsPress(KK_S) || InputKeyboard_IsPress(KK_DOWN))
	{
		velocity.y += 1.0f;
	}

	//Normalize velocity vector to prevent faster diagonal movement
	XMVECTOR vel = XMLoadFloat2(&velocity);
	XMVECTOR len = XMVector2Length(vel);

	if (XMVectorGetX(len) > 0.0f)
	{
		vel = XMVector2Normalize(vel);
		XMStoreFloat2(&velocity, vel);

		//Apply speed and delta time
		g_playerX += velocity.x * g_playerSpeed * delta_time;
		g_playerY += velocity.y * g_playerSpeed * delta_time;

		//g_playerX = std::clamp(g_playerX, -g_playerOffsetX, SCREEN_WIDTH - g_player_width + g_playerOffsetX);
		//g_playerY = std::clamp(g_playerY, -g_playerOffsetY, SCREEN_HEIGHT - g_player_height + g_playerOffsetY);
		
		//Clamp to screen bounds
		if (g_playerX < -g_playerOffsetX) g_playerX = -g_playerOffsetX;
		if (g_playerX > SCREEN_WIDTH - g_player_width + g_playerOffsetX) g_playerX = SCREEN_WIDTH - g_player_width + g_playerOffsetX;
		if (g_playerY < -g_playerOffsetY) g_playerY = -g_playerOffsetY;
		if (g_playerY > SCREEN_HEIGHT - g_player_height + g_playerOffsetY) g_playerY = SCREEN_HEIGHT - g_player_height + g_playerOffsetY;
	}

	// Fire bullet when space is triggered
	if (InputKeyboard_IsTrigger(KK_SPACE))
	{
		GamePlayerBullet_Create(g_playerX + g_player_width, g_playerY + g_player_height * 0.5f);
	}
}

void GamePlayer_Draw()
{
	//座標にプレイヤーを描画する
	Sprite_SetFilter(kSpriteFilter_Linear);

	Sprite_Draw(g_player_texture_ID, g_playerX, g_playerY, g_player_width, g_player_height);		
}

float GamePlayer_GetPosX()
{
	return g_playerX;
}

float GamePlayer_GetPosY()
{
	return g_playerY;
}

float GamePlayer_GetSpeed()
{
	return g_playerSpeed;
}

void GamePlayer_SetSpeed(float speed)
{
	if (speed < 0.0f)
	{
		speed = 0.0f;
	}

	g_playerSpeed = speed;
}
