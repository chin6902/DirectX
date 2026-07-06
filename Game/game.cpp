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
#include "collision_debug.h"
#include "game_impact.h"
#include "flipbook_animation.h"

enum State
{
	
};

static int g_TextureId_Bg = TEXTURE_INVALID_ID;

constexpr float startX = 50.0f;
constexpr float startY = (SCREEN_HEIGHT - 64.0f) * 0.5f;

void Collision_CheckPlayerBulletsVsEnemies();

void Game_Initialize()
{

	g_TextureId_Bg = Texture_Load(L"assets/textures/Background.png", false);

    GamePlayer_Initialize(startX, startY);
    GamePlayerBullet_Initialize();
	EnemySpawner_Initialize();
	GameEnemy_Initialize();
	Game_Impact_Create();
	Game_Impact_Initialize();

#ifdef _DEBUG
	Collision_Debug_Initialize();
#endif
}

void Game_Finalize()
{
#ifdef _DEBUG
	Collision_Debug_Finalize();
#endif
	Game_Impact_Finalize();
	GamePlayer_Finalize();
	GamePlayerBullet_Finalize();
	GameEnemy_Finalize();
	Game_Impact_Finalize();
	Texture_Release(g_TextureId_Bg);
}

void Game_Update(float delta_time)
{
	GamePlayer_Update(delta_time);
	GamePlayerBullet_Update(delta_time);	
	EnemySpawner_Update(delta_time);
	GameEnemy_Update(delta_time);
	FlipBookAnimation_Update(delta_time);

	Collision_CheckPlayerBulletsVsEnemies();
	// Additional collision checks can be added here, such as player vs enemies, etc.

	GamePlayerBullet_CleanUp();
	GameEnemy_CleanUp();
	Game_Impact_Update(delta_time);
}

void Game_Draw()
{
	Sprite_SetFilter(kSpriteFilter_Linear);
	Sprite_Draw(g_TextureId_Bg, 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f, 0.0f, Texture_GetWidth(g_TextureId_Bg), Texture_GetHeight(g_TextureId_Bg));
	GamePlayer_Draw();
	GamePlayerBullet_Draw();
	GameEnemy_Draw();
	Game_Impact_Draw();
}

void Collision_CheckPlayerBulletsVsEnemies()
{
	for (int player_bullet_index = 0; player_bullet_index < GamePlayerBullet_GetActiveCount(); player_bullet_index++)
	{
		CollisionCircle bulletCircle = GamePlayerBullet_GetCollisionCircle(player_bullet_index);

		for (int enemy_index = 0; enemy_index < GameEnemy_GetActiveCount(); enemy_index++)
		{
			CollisionCircle enemyCircle = GameEnemy_GetCollisionCircle(enemy_index);

			if (Collision_IsOverlap(bulletCircle, enemyCircle))
			{
				Game_Impact_Trigger(GameEnemy_GetExplosionType(enemy_index), enemyCircle.position.x, enemyCircle.position.y);

				GamePlayerBullet_Destroy(player_bullet_index);
				GameEnemy_Destroy(enemy_index);
				break;
			}
		}
	}
}
