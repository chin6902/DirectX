/*============================================================================
Contents   :  [boss_visual.h]

Author     : Chin Qing You
LastUpdate : 2026/09/04
-----------------------------------------------------------------------------

============================================================================*/
#ifndef BOSS_VISUAL_H
#define BOSS_VISUAL_H

#include <DirectXMath.h>

#include "sprite.h"
#include "vector2.h"
#include "game_enemy.h"  

// Boss variants, in wave order: 1 = basic, 2 = laser, 3 = wall
enum BossVariant
{
	BOSS_VARIANT_BASIC,     // Slime1
	BOSS_VARIANT_LASER,     // Slime3
	BOSS_VARIANT_WALL,      // Slime2
	BOSS_VARIANT_COUNT,
};

enum BossClip
{
	BOSS_CLIP_IDLE,
	BOSS_CLIP_RUN,
	BOSS_CLIP_ATTACK,
	BOSS_CLIP_DEATH,
	BOSS_CLIP_COUNT,
};

enum BossFacing
{
	BOSS_FACE_DOWN,
	BOSS_FACE_UP,
	BOSS_FACE_LEFT,
	BOSS_FACE_RIGHT,
	BOSS_FACE_COUNT,
};

void BossVisual_Initialize();
void BossVisual_Finalize();

int   BossVisual_GetFrameCount(int variant, BossClip clip);
float BossVisual_GetFrameTime(int variant, BossClip clip);

int   BossVisual_FacingFromDir(const Vector2& dir);

void BossVisual_DrawShadow(const Vector2& world_pos, float radius,
	float jump_height, float arc_height);

void BossVisual_DrawBody(int variant, BossClip clip,
	int frame, int facing,
	const Vector2& world_pos, float draw_size,
	float jump_height,
	const SpriteDrawParams& params);

void BossVisual_DrawShield(const Vector2& world_pos, float radius,
	float jump_height, ElementType element);

DirectX::XMFLOAT3 BossVisual_ElementColor(ElementType element);

#endif