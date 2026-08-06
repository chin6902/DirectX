/*============================================================================
Contents   :  [game.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/24
-----------------------------------------------------------------------------

============================================================================*/
#include "Audio.h"
#include "input_keyboard.h"
#include "texture.h"
#include "sprite.h"
#include "config.h"
#include "flipbook_animation.h"

#include "game.h"
#include "camera.h"
#include "game_player.h"
#include "game_playerBullet.h"
#include "enemy_spawner.h"
#include "game_enemy.h"
#include "collision.h"
#include "collision_debug.h"
#include "game_impact.h"
#include "game_score.h"
#include "scene.h"
#include "fade.h"
#include "game_stage.h"
#include "game_skill.h"

enum State
{
	STATE_PLAYING,
	STATE_PAUSE,
	STATE_GAMEOVER,
	STATE_GAMECLEAR,
};

static State g_gameState = STATE_PLAYING;
static bool g_IsChangeScene = false;

static int g_TextureId_Bg = TEXTURE_INVALID_ID;
static int g_BgmId = -1;

static int g_score = 0;

#ifdef _DEBUG
	int debug_result = 0;
#endif

void Collision_CheckPlayerBulletsVsEnemies();

void Game_Initialize()
{
	g_gameState = STATE_PLAYING;

	g_TextureId_Bg = Texture_Load(L"assets/textures/Background.png");
	g_BgmId = LoadAudio("assets/sounds/bgm.wav");
	g_score = 0;

	GamePlayer_Initialize(GameStage_GetWidth() * 0.5f, GameStage_GetHeight() * 0.5f);
	GameSkill_Initialize();
/*    GamePlayerBullet_Initialize();
	EnemySpawner_Initialize();
	GameEnemy_Initialize();
	GameImpact_Create();
	GameImpact_Initialize();
	GameScore_Initialize(6); // Initialize score display with 6 digits*/
	GameStage_Initialize();

	//PlayAudio(g_BgmId, true);

#ifdef _DEBUG
	Collision_Debug_Initialize();
#endif

	Fade_Start(FADE_IN, 1.0f, { 0.0f, 0.0f, 0.0f, 0.0f });
	g_IsChangeScene = false;
}

void Game_Finalize()
{
#ifdef _DEBUG
	Collision_Debug_Finalize();
#endif
	GameStage_Finalize();
	/*GameScore_Finalize();
	GameImpact_Finalize();*/
	GameSkill_Finalize();
	GamePlayer_Finalize();
	/*GamePlayerBullet_Finalize();
	GameEnemy_Finalize();*/
	UnloadAudio(g_BgmId);
	Texture_Release(g_TextureId_Bg);
}

void Game_Update(float delta_time)
{
	if (InputKeyboard_IsTrigger(KK_P))
	{
		g_gameState = (g_gameState == STATE_PLAYING) ? STATE_PAUSE : STATE_PLAYING;
	}
	
	switch (g_gameState)
	{
	case STATE_PLAYING:
		GamePlayer_Update(delta_time);
		GameSkill_Update(delta_time);
		Camera_Update(delta_time);
		//GamePlayerBullet_Update(delta_time);
		//EnemySpawner_Update(delta_time);
		//GameEnemy_Update(delta_time);
		FlipBookAnimation_Update(delta_time);

		//Collision_CheckPlayerBulletsVsEnemies();
		// Additional collision checks can be added here, such as player vs enemies, etc.

		/*GamePlayerBullet_CleanUp();
		GameEnemy_CleanUp();
		GameImpact_Update(delta_time);
		GameScore_Update(delta_time);*/
		break;

	case STATE_PAUSE:
		break;
	}

	debug_result = g_score;

	if (!g_IsChangeScene)
	{
		/*
		if (debug_result == 100)
		{
			Fade_Start(FADE_OUT, 1.0f, { 0.0f, 0.0f, 0.0f, 1.0f });
			g_IsChangeScene = true;
		}
		*/
	}
	else
	{
		if (Fade_IsFinished())
		{
			Scene_SetNextScene(SCENE_RESULT);
		}
	}
}

void Game_Draw()
{
	GameStage_Draw();
	GameStage_DebugDraw();
	Sprite_SetFilter(kSpriteFilter_Linear);
	GamePlayer_Draw();
	GameSkill_Draw();
	/*GamePlayerBullet_Draw();
	GameEnemy_Draw();
	GameImpact_Draw();
	Sprite_SetFilter(kSpriteFilter_Point);

	GameScore_Draw(1200.0f, 10.0f, 0.5f);*/
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
				GameImpact_Trigger(GameEnemy_GetExplosionType(enemy_index), enemyCircle.position.x, enemyCircle.position.y);

				GamePlayerBullet_Destroy(player_bullet_index);
				GameEnemy_Destroy(enemy_index);

				g_score += 10;
				GameScore_SetScore(g_score);
				break;
			}
		}
	}
}
