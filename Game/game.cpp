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
#include "game_progress.h"
#include "game_levelup.h"
#include "game_text.h"
#include "game_ui.h"
#include "debug_ostream.h"

enum State
{
	STATE_PLAYING,
	STATE_PAUSE,
	STATE_LEVELUP,
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

void Collision_CheckPlayerVsEnemies();

void Game_Initialize()
{
	g_gameState = STATE_PLAYING;

	g_TextureId_Bg = Texture_Load(L"assets/textures/Background.png");
	g_BgmId = LoadAudio("assets/sounds/bgm.wav");
	g_score = 0;

	//Collision_Debug_Initialize();

	GameText_Initialize();
	GameLevelUp_Initialize();
	GameProgress_Initialize();
	GamePlayer_Initialize(GameStage_GetWidth() * 0.5f, GameStage_GetHeight() * 0.5f);
	GameSkill_Initialize();
	EnemySpawner_Initialize();
	GameEnemy_Initialize();
	/*
	GameImpact_Create();
	GameImpact_Initialize();
	GameScore_Initialize(6); // Initialize score display with 6 digits*/
	GameStage_Initialize();
	GameUI_Initialize();

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
	GameUI_Finalize();
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

	if (InputKeyboard_IsTrigger(KK_O))
	{
		GameProgress_AddXP(40);
	}
	
	switch (g_gameState)
	{
	case STATE_PLAYING:
		GamePlayer_Update(delta_time);
		GameSkill_Update(delta_time);
		Camera_Update(delta_time);
		EnemySpawner_Update(delta_time);
		GameEnemy_Update(delta_time);
		FlipBookAnimation_Update(delta_time);

#ifdef _DEBUG
		// spawner debugger
		if (InputKeyboard_IsTrigger(KK_F1))
		{
			GameEnemy_Create(EnemyType_CHASER,
				{ GamePlayer_GetPosX() + 300.0f, GamePlayer_GetPosY() });
		}
		if (InputKeyboard_IsTrigger(KK_F2))
		{
			GameEnemy_Create(EnemyType_ORBITER,
				{ GamePlayer_GetPosX() + 300.0f, GamePlayer_GetPosY() });
		}
		if (InputKeyboard_IsTrigger(KK_F3))   // a small crowd, for blob-watching
		{
			for (int i = 0; i < 10; i++)
			{
				const float a = 6.2831853f * (i / 10.0f);
				GameEnemy_Create(EnemyType_CHASER,
					Vector2{ GamePlayer_GetPosX(), GamePlayer_GetPosY() }
				+ Vector2_FromAngle(a) * 400.0f);
			}
		}
#endif

		if (GameProgress_IsLevelPending())
		{
			g_gameState = STATE_LEVELUP;
		}

		Collision_CheckPlayerVsEnemies();
		// Additional collision checks can be added here, such as player vs enemies, etc.

		GameUI_Update(delta_time);

		GameEnemy_CleanUp();
		/*GameImpact_Update(delta_time);
		GameScore_Update(delta_time);*/
		break;

	case STATE_PAUSE:
		break;

	case STATE_LEVELUP:
		GameLevelUp_Update(delta_time);
		if (!GameProgress_IsLevelPending()) { g_gameState = STATE_PLAYING; }
		break;
	}

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
	GameEnemy_Draw();
	GameSkill_Draw();
	GameUI_Draw();
	//GameImpact_Draw();
	Sprite_SetFilter(kSpriteFilter_Point);

	if (g_gameState == STATE_LEVELUP) { GameLevelUp_Draw(); }

	//GameScore_Draw(1200.0f, 10.0f, 0.5f);
}

void Collision_CheckPlayerVsEnemies()
{
	if (GamePlayer_IsInvincible() || GamePlayer_IsDead()) { return; }
	
	const CollisionCircle player_circle = GamePlayer_GetCollisionCircle();
	const Vector2 player_pos = GamePlayer_GetPos();

	for (int i = 0; i < GameEnemy_GetActiveCount(); i++)
	{
		const CollisionCircle enemy_circle = GameEnemy_GetCollisionCircle(i);
		if (!Collision_IsOverlap(player_circle, enemy_circle)) { continue; }

		PlayerHit hit;
		hit.damage = GameEnemy_GetContactDamage(i);
		hit.knockback_speed = 600.0f;
		hit.knockback_time = 0.50f;

		const Vector2 away = player_pos - GameEnemy_GetPos(i);
		hit.direction = (away.LengthSq() < 0.01f) ? Vector2{ 1.0f, 0.0f } : Vector2_Normalize(away);

		GamePlayer_TakeHit(hit);
		return;
	}
}
