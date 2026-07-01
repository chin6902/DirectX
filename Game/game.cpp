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
#include "enemy_spawner.h"
#include "game_enemy.h"
#include "collision.h"

enum State
{
	
};

static int g_TextureId_Bg = TEXTURE_INVALID_ID;

constexpr float startX = 50.0f;
constexpr float startY = (SCREEN_HEIGHT - 64.0f) * 0.5f;

void Game_Initialize()
{

	g_TextureId_Bg = Texture_Load(L"assets/textures/Background.png", false);

    GamePlayer_Initialize(startX, startY);
    GamePlayerBullet_Initialize();
	EnemySpawner_Initialize();
	GameEnemy_Initialize();
}

void Game_Finalize()
{
	GamePlayer_Finalize();
	GamePlayerBullet_Finalize();
	GameEnemy_Finalize();
	Texture_Release(g_TextureId_Bg);
}

void Game_Update(float delta_time)
{
	GamePlayer_Update(delta_time);
	GamePlayerBullet_Update(delta_time);	
	EnemySpawner_Update(delta_time);
	GameEnemy_Update(delta_time);

	// Spatial collision detection
	Grid_Clear();

	// Add all enemies to the grid
	// Note: You'll need to expose enemy positions from game_enemy module
	// For now, this shows the pattern

	// Query nearby enemies for each bullet and check collision
	// This will be populated once you expose bullet/enemy data
}

void Game_Draw()
{
	Sprite_SetFilter(kSpriteFilter_Linear);
	Sprite_Draw(g_TextureId_Bg, 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f, 0.0f, Texture_GetWidth(g_TextureId_Bg), Texture_GetHeight(g_TextureId_Bg));
	GamePlayer_Draw();
	GamePlayerBullet_Draw();
	GameEnemy_Draw();
}
