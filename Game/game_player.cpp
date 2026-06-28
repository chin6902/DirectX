/*============================================================================
Contents   :  [game_player.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/24
-----------------------------------------------------------------------------

============================================================================*/
#include "game_player.h"
#include "texture.h"
#include "sprite.h" //"flipbook animation"
#include "input_keyboard.h"
#include "config.h"
#include <DirectXMath.h>	

//player texture ID
int g_player_texture_ID = -1;
float g_player_width = 128.0f;
float g_player_height = 64.0f;
		
//player position
float g_playerX = 0.0f;
float g_playerY = 0.0f;
float g_playerOffsetX = 30.0f;
float g_playerOffsetY = 10.0f;
float g_playerSpeed = 200.0f;

void GamePlayer_Initialize(float startX, float startY)
{				
	//プレイヤーのテクスチャを読み込む
	g_player_texture_ID = Texture_Load(L"assets/textures/player.png");
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
	//Calculate movement direction
	DirectX::XMFLOAT2 velocity = { 0.0f, 0.0f };

	if (InputKeyboard_IsPress(KK_A))
	{
		velocity.x -= 1.0f;
	}
	if (InputKeyboard_IsPress(KK_D))
	{
		velocity.x += 1.0f;
	}
	if (InputKeyboard_IsPress(KK_W))
	{
		velocity.y -= 1.0f;
	}
	if (InputKeyboard_IsPress(KK_S))
	{
		velocity.y += 1.0f;
	}

	//Normalize velocity vector to prevent faster diagonal movement
	DirectX::XMVECTOR vel = DirectX::XMLoadFloat2(&velocity);
	DirectX::XMVECTOR len = DirectX::XMVector2Length(vel);

	if (DirectX::XMVectorGetX(len) > 0.0f)
	{
		vel = DirectX::XMVector2Normalize(vel);
		DirectX::XMStoreFloat2(&velocity, vel);

		//Apply speed and delta time
		g_playerX += velocity.x * g_playerSpeed * delta_time;
		g_playerY += velocity.y * g_playerSpeed * delta_time;

		//Clamp to screen bounds
		if (g_playerX < -g_playerOffsetX) g_playerX = -g_playerOffsetX;
		if (g_playerX > SCREEN_WIDTH - g_player_width + g_playerOffsetX) g_playerX = SCREEN_WIDTH - g_player_width + g_playerOffsetX;
		if (g_playerY < -g_playerOffsetY) g_playerY = -g_playerOffsetY;
		if (g_playerY > SCREEN_HEIGHT - g_player_height + g_playerOffsetY) g_playerY = SCREEN_HEIGHT - g_player_height + g_playerOffsetY;
	}
}

void GamePlayer_Draw()
{
	//座標にプレイヤーを描画する
	Sprite_SetFilter(kSpriteFilter_Linear);

	Sprite_Draw(g_player_texture_ID, g_playerX, g_playerY, g_player_width, g_player_height);		
}

