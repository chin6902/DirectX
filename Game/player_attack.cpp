/*============================================================================
Contents   :  [player_attack.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/01
-----------------------------------------------------------------------------

============================================================================*/
#include <cmath>

#include "player_attack.h"
#include "game_player.h"
#include "game_enemy.h"
#include "game_impact.h"
#include "flipbook_animation.h"
#include "texture.h"
#include "vector2.h"
#include "camera.h"
#include "collision_debug.h"

// attack parameters
static constexpr float ATTACK_COOLDOWN = 3.0f;
static constexpr float ATTACK_RANGE = 150.0f;
static constexpr int   ATTACK_DAMAGE = 1;
static constexpr float ATTACK_HALF_ANGLE = 0.7853f;            
static const float COS_HALF_ANGLE = cosf(ATTACK_HALF_ANGLE);   

static float g_CooldownTimer = 0.0f;
static float g_SwingFlashTimer = 0.0f;  

// slash visual
static constexpr int   SLASH_CELL_W = 200;
static constexpr int   SLASH_CELL_H = 400;
static constexpr int   SLASH_FRAME_MAX = 4;
static constexpr int   SLASH_COLS = 4;
static constexpr float SLASH_FRAME_TIME = 0.05f;
static constexpr float SLASH_DRAW_W = 165.0f; 
static constexpr float SLASH_DRAW_H = 300.0f;

static int     g_slash_texture_id = -1;
static int     g_slash_anim_id = -1;
static bool    g_slash_active = false;
static float   g_slash_angle = 0.0f;
static Vector2 g_slash_offset = {};

void PlayerAttack_Initialize()
{
	g_CooldownTimer = 0.0f;
	g_SwingFlashTimer = 0.0f;
	g_slash_active = false;

	if (g_slash_texture_id != TEXTURE_INVALID_ID)
	{
		g_slash_anim_id = FlipBookAnimation_Create(
			g_slash_texture_id,
			SLASH_CELL_W, SLASH_CELL_H,
			SLASH_FRAME_MAX, SLASH_COLS,
			SLASH_FRAME_TIME);
		FlipBookAnimation_SetMode(g_slash_anim_id, AnimPlayMode::ONE_SHOT);
	}
}

void PlayerAttack_Finalize()
{
	if (g_slash_anim_id != -1)
	{
		FlipBookAnimation_Destroy(g_slash_anim_id);
		g_slash_anim_id = -1;
	}

}

void PlayerAttack_Update(float delta_time)
{
	if (g_SwingFlashTimer > 0.0f) { g_SwingFlashTimer -= delta_time; }

	g_CooldownTimer -= delta_time;
	if (g_CooldownTimer > 0.0f)
	{
		return;
	}

	g_CooldownTimer = ATTACK_COOLDOWN;
	g_SwingFlashTimer = 0.15f;

	const Vector2 origin = { GamePlayer_GetPosX(), GamePlayer_GetPosY() };
	const Vector2 aim = GamePlayer_GetAimDir();

	if (g_slash_anim_id != -1)
	{
		g_slash_active = true;
		g_slash_angle = Vector2_ToAngle(aim);
		g_slash_offset =	 aim * (ATTACK_RANGE * 0.5f);
		FlipBookAnimation_SetFrame(g_slash_anim_id, 0);
	}

	for (int i = 0; i < GameEnemy_GetActiveCount(); i++)
	{
		const CollisionCircle cc = GameEnemy_GetCollisionCircle(i);
		const Vector2 to_enemy = Vector2{ cc.position.x, cc.position.y } - origin;

		// Test 1: range
		if (to_enemy.LengthSq() > ATTACK_RANGE * ATTACK_RANGE) { continue; }
		// Test 2: angle
		if (Vector2_Dot(aim, Vector2_Normalize(to_enemy)) < COS_HALF_ANGLE) { continue; }

		HitInfo hit;
		hit.damage = ATTACK_DAMAGE;
		hit.knockback_speed = 250.0f;      
		hit.knockback_time = 0.12f;
		hit.hitstun_time = 0.20f;
		hit.direction = Vector2_Normalize(to_enemy);

		if (GameEnemy_ApplyHit(i, hit))
		{
		}
	}
}

void PlayerAttack_Draw()
{
	// --- Slash effect ---
	if (g_slash_active)
	{
		if (FlipBookAnimation_IsFinished(g_slash_anim_id))
		{
			g_slash_active = false;
		}
		else
		{
			SpriteDrawParams p;
			p.angle = g_slash_angle;
			
			const Vector2 g_slash_pos = Vector2{ GamePlayer_GetPosX(), GamePlayer_GetPosY() } + g_slash_offset;

			FlipBookAnimation_Draw(
				g_slash_anim_id,
				Camera_WorldToScreenX(g_slash_pos.x - SLASH_DRAW_W * 0.5f),
				Camera_WorldToScreenY(g_slash_pos.y - SLASH_DRAW_H * 0.5f),
				SLASH_DRAW_W, SLASH_DRAW_H,
				p);
		}
	}
}
