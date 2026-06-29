/*============================================================================
Contents   :  [game.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/24
-----------------------------------------------------------------------------

============================================================================*/
#include "game.h"
#include "texture.h"
#include "sprite.h"
#include "config.h"
#include "game_player.h"
#include "game_playerBullet.h"


static int g_TextureId_Bg = TEXTURE_INVALID_ID;

void Game_Initialize()
{

	g_TextureId_Bg = Texture_Load(L"assets/textures/Background.png", false);

	constexpr float startX = 50.0f;
	constexpr float startY = (SCREEN_HEIGHT - 64.0f) * 0.5f;
    GamePlayer_Initialize(startX, startY);
    GamePlayerBullet_Initialize();
}

void Game_Finalize()
{
	GamePlayer_Finalize();
	GamePlayerBullet_Finalize();
	Texture_Release(g_TextureId_Bg);
}

void Game_Update(float delta_time)
{
	GamePlayer_Update(delta_time);
	GamePlayerBullet_Update(delta_time);
}

void Game_Draw()
{
	Sprite_SetFilter(kSpriteFilter_Linear);
	Sprite_Draw(g_TextureId_Bg, 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f, 0.0f, Texture_GetWidth(g_TextureId_Bg), Texture_GetHeight(g_TextureId_Bg));
	GamePlayer_Draw();
	GamePlayerBullet_Draw();
}
