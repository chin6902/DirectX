/*============================================================================
Contents   :  [game.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/24
-----------------------------------------------------------------------------

============================================================================*/
#include "input_keyboard.h"
#include "texture.h"
#include "sprite.h"
#include "config.h"
#include "flipbook_animation.h"

#include "game.h"
#include "result.h"
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
#include "game_expgem.h"
#include "enemy_projectile.h"
#include "game_boss.h"
#include "boss_wall.h"
#include "enemy_formation.h"
#include "game_damagenumber.h"
#include "flow_field.h"
#include "game_item.h"
#include "wave_banner.h"
#include "game_audio.h"
#include "mouse_ui.h"

enum State
{
	STATE_PLAYING,
	STATE_PAUSE,
	STATE_LEVELUP,
	STATE_GAMEOVER,
	STATE_GAMECLEAR,
};

static constexpr float END_HOLD_GAMEOVER = 1.40f;
static constexpr float END_HOLD_CLEAR = 2.00f;
static bool g_RunStarted = false;

static State g_gameState = STATE_PLAYING;
static bool  g_IsChangeScene = false;
static float g_EndTimer = 0.0f;
static float g_RunTime = 0.0f;
static int   g_BgmId = -1;

void Collision_CheckPlayerVsEnemies();

void Game_Initialize()
{
	g_gameState = STATE_PLAYING;
	g_IsChangeScene = false;  
	g_EndTimer = 0.0f;
	g_RunTime = 0.0f;
	g_RunStarted = false;

	GameText_Initialize();
	GameLevelUp_Initialize();
	GameProgress_Initialize();
	GameStage_Initialize();
	GamePlayer_Initialize(GameStage_GetWidth() * 0.5f, GameStage_GetHeight() * 0.5f);
	GameSkill_Initialize();
	FlowField_Initialize();
	EnemySpawner_Initialize();
	GameEnemy_Initialize();
	EnemyProjectile_Initialize();
	GameExpGem_Initialize();
	GameBoss_Initialize();
	BossWall_Initialize();
	EnemyFormation_Initialize();
	GameDamageNumber_Initialize();
	GameItem_Initialize();

	// --- UI ---
	GameUI_Initialize();
	MouseUI_Initialize();
	WaveBanner_Initialize();

#ifdef _DEBUG
	Collision_Debug_Initialize();
#endif

	Fade_Start(FADE_IN, 1.0f, { 0.0f, 0.0f, 0.0f, 0.0f });
}

void Game_Finalize()
{
#ifdef _DEBUG
	Collision_Debug_Finalize();
#endif
	WaveBanner_Finalize();
	MouseUI_Finalize();
	GameUI_Finalize();
	GameStage_Finalize();
	GameItem_Finalize();
	GameDamageNumber_Finalize();
	GameEnemy_Finalize();
	EnemyProjectile_Finalize();
	GameExpGem_Finalize();
	GameBoss_Finalize();
	BossWall_Finalize();
	EnemyFormation_Finalize();
	FlowField_Finalize();
	GameText_Finalize();
	GameSkill_Finalize();
	GamePlayer_Finalize();
}

static void EnterEndState(bool cleared)
{
	g_gameState = cleared ? STATE_GAMECLEAR : STATE_GAMEOVER;
	g_EndTimer = cleared ? END_HOLD_CLEAR : END_HOLD_GAMEOVER;

	GameAudio_StopMusic();
	GameAudio_Play(cleared ? SND_WIN_VOICE : SND_LOSE_VOICE);
	GameAudio_Play(cleared ? SND_GAME_CLEAR : SND_GAME_OVER);

	Result_SetOutcome(cleared, GameProgress_GetLevel(), g_RunTime);
}

static bool IsRunCleared()
{
	return g_RunStarted && EnemySpawner_IsFinished() && GameEnemy_GetActiveCount() == 0 && !GameBoss_AnyActive();
}

static void UpdateEndHold(float delta_time)
{
	Camera_Update(delta_time);
	GameSkill_Update(delta_time);
	GameEnemy_Update(delta_time);
	EnemyProjectile_Update(delta_time);
	GameBoss_Update(delta_time);
	EnemyFormation_Update(delta_time);
	GameDamageNumber_Update(delta_time);
	FlipBookAnimation_Update(delta_time);
	GameUI_Update(delta_time);
	GamePlayer_Update(delta_time);
	GameEnemy_CleanUp();
	GameEnemy_BuildGrid();

	if (g_EndTimer > 0.0f)
	{
		g_EndTimer -= delta_time;
		return;
	}

	if (!g_IsChangeScene)
	{
		Fade_Start(FADE_OUT, 1.0f, { 0.0f, 0.0f, 0.0f, 1.0f });
		g_IsChangeScene = true;
	}
}


void Game_Update(float delta_time)
{
	if (InputKeyboard_IsTrigger(KK_P))
	{
		if (g_gameState == STATE_PLAYING) { g_gameState = STATE_PAUSE; }
		else if (g_gameState == STATE_PAUSE) { g_gameState = STATE_PLAYING; }
	}

#ifdef _DEBUG
	if (InputKeyboard_IsTrigger(KK_O)) { GameProgress_AddXP(50); }
	if (InputKeyboard_IsTrigger(KK_F1)) { GameBoss_Spawn(GamePlayer_GetPos() + Vector2{ 300.0f, 0.0f }, 1, false); }
	if (InputKeyboard_IsTrigger(KK_F2)) { GameBoss_Spawn(GamePlayer_GetPos() + Vector2{ 300.0f, 0.0f }, 2, false); }
	if (InputKeyboard_IsTrigger(KK_F3)) { GameBoss_Spawn(GamePlayer_GetPos() + Vector2{ 300.0f, 0.0f }, 3, false); }
	if (InputKeyboard_IsTrigger(KK_T))  { EnterEndState(true); }
	if (InputKeyboard_IsTrigger(KK_Y))  { EnterEndState(false); }
#endif

	switch (g_gameState)
	{
	case STATE_PLAYING:
		g_RunTime += delta_time;

		// --- bgm ---
		GameAudio_PlayMusic(GameAudio_TrackForWave(EnemySpawner_GetWaveNumber(), EnemySpawner_IsBossWave()));
		GameAudio_UpdateMusic(delta_time);

		// --- game update ---
		GamePlayer_Update(delta_time);
		GameSkill_Update(delta_time);
		Camera_Update(delta_time);
		EnemySpawner_Update(delta_time);
		FlowField_Update(GamePlayer_GetPos());
		GameEnemy_Update(delta_time);
		FlipBookAnimation_Update(delta_time);
		GameExpGem_Update(delta_time);
		EnemyProjectile_Update(delta_time);
		GameBoss_Update(delta_time);
		BossWall_Update(delta_time);
		EnemyFormation_Update(delta_time);
		GameDamageNumber_Update(delta_time);
		GameItem_Update(delta_time);

		// --- collision check ---
		Collision_CheckPlayerVsEnemies();

		// --- UI ---
		GameUI_Update(delta_time);
		MouseUI_Update(delta_time);
		WaveBanner_Update(delta_time);

		GameEnemy_CleanUp();
		GameEnemy_BuildGrid();

		if (!g_RunStarted && (GameEnemy_GetActiveCount() > 0 || GameBoss_AnyActive()))
		{
			g_RunStarted = true;
		}

		if (GamePlayer_IsDead())
		{
			EnterEndState(false);
		}
		else if (IsRunCleared())
		{
			EnterEndState(true);
		}
		else if (GameProgress_IsLevelPending())
		{
			GameAudio_Play(SND_LEVEL_UP);
			g_gameState = STATE_LEVELUP;
		}
		break;

	case STATE_PAUSE:
		break;

	case STATE_LEVELUP:
		GameLevelUp_Update(delta_time);
		if (!GameProgress_IsLevelPending()) { g_gameState = STATE_PLAYING; }
		break;

	case STATE_GAMEOVER:
	case STATE_GAMECLEAR:
		UpdateEndHold(delta_time);
		break;
	}

	if (g_IsChangeScene && Fade_IsFinished())
	{
		Scene_SetNextScene(SCENE_RESULT);
	}
}

void Game_Draw()
{
	GameStage_Draw();
	GameStage_DebugDraw();

	Sprite_SetFilter(kSpriteFilter_Linear);

	GamePlayer_Draw();
	EnemyProjectile_Draw();
	GameExpGem_Draw();
	GameSkill_DrawUnder();
	GameEnemy_Draw();
	GameSkill_Draw();
	BossWall_Draw();
	GameBoss_Draw();
	EnemyFormation_Draw();
	GameDamageNumber_Draw();
	GameItem_Draw();

	// --- UI ---
	GameUI_Draw();
	GameSkill_DrawOverlay();
	MouseUI_Draw();
	GameBoss_DrawUI();
	WaveBanner_DrawCounter();
	WaveBanner_Draw();

	Sprite_SetFilter(kSpriteFilter_Point);

	if (g_gameState == STATE_LEVELUP) { GameLevelUp_Draw(); }

	if (g_gameState == STATE_PAUSE)
	{
		GameUI_DrawScreenRect(0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT,
			{ 0.0f, 0.0f, 0.0f }, 0.60f);
		GameText_DrawCentered(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f - 40.0f,
			"PAUSED", 1.0f, { 1.0f, 1.0f, 1.0f });
		GameText_DrawCentered(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f + 40.0f,
			"P  RESUME", 0.45f, { 0.65f, 0.68f, 0.75f });
	}

	if (g_gameState == STATE_GAMEOVER)
	{
		GameText_DrawCentered(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f,
			"YOU DIED", 1.4f, { 0.90f, 0.25f, 0.25f });
	}
	else if (g_gameState == STATE_GAMECLEAR)
	{
		GameText_DrawCentered(SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f,
			"SURVIVED", 1.4f, { 1.00f, 0.90f, 0.45f });
	}

	Sprite_Flush();
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

		if (GameEnemy_UsesAttack(i)) { continue; }

		PlayerHit hit;
		hit.damage = GameEnemy_GetContactDamage(i);
		hit.knockback_speed = 600.0f;
		hit.knockback_time = 0.50f;

		const Vector2 away = player_pos - GameEnemy_GetPos(i);
		hit.direction = (away.LengthSq() < 0.01f) ? Vector2{ 1.0f, 0.0f } : Vector2_Normalize(away);

		GamePlayer_TakeHit(hit);
		return;
	}

	for (int i = 0; i < BOSS_MAX; i++)
	{
		if (!GameBoss_IsActive(i) || !GameBoss_IsGrounded(i)) { continue; }

		const Vector2 boss_pos = GameBoss_GetPos(i);
		const CollisionCircle boss_circle{ { boss_pos.x, boss_pos.y }, GameBoss_GetRadius(i) };
		if (!Collision_IsOverlap(player_circle, boss_circle)) { continue; }

		PlayerHit hit;
		hit.damage = static_cast<int>(BOSS_CONTACT_DAMAGE);
		hit.knockback_speed = 820.0f;
		hit.knockback_time = 0.70f;

		const Vector2 away = player_pos - boss_pos;
		hit.direction = (away.LengthSq() < 0.01f) ? Vector2{ 1.0f, 0.0f } : Vector2_Normalize(away);
		GamePlayer_TakeHit(hit);
		return;
	}
}
