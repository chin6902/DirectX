/*============================================================================
Contents   :  [game_playerBullet.cpp]

Author     : Chin Qing You
LastUpdate : 2026/06/29
-----------------------------------------------------------------------------

============================================================================*/
#include "game_playerBullet.h"
#include "config.h"
#include "texture.h"
#include "sprite.h"
#include "collision.h"
#include "collision_debug.h"

static int g_bullet_texture_ID = -1;
static constexpr int   BULLET_CELL_W = 197;
static constexpr int   BULLET_CELL_H = 148;
static constexpr int   BULLET_CELL_OFFSET_Y = 5;                        
static constexpr int   BULLET_CONTENT_H = BULLET_CELL_H - BULLET_CELL_OFFSET_Y * 2; 
static constexpr int   BULLET_FRAME_MAX = 4;
static constexpr int   BULLET_COLS = 1;
static constexpr float BULLET_FRAME_TIME = 1.0f / 5.0f;  

struct Bullet 
{
	float posX;
	float posY;
	int anim_frame;
	float anim_timer;
	bool isDestroy;
};

static constexpr int MAX_BULLETS = 100;
static Bullet g_Bullets[MAX_BULLETS]{};
static int g_BulletFireCount = 0;

static constexpr float BULLET_WIDTH = 64.0f;
static constexpr float BULLET_HEIGHT = BULLET_WIDTH * (float(BULLET_CONTENT_H) / float(BULLET_CELL_W));
static constexpr float BULLET_SPEED = 600.0f; 

void GamePlayerBullet_Initialize()
{
	g_bullet_texture_ID = Texture_Load(L"assets/textures/BulletAnimated.png");
}

void GamePlayerBullet_Finalize()
{
	Texture_Release(g_bullet_texture_ID);
}

void GamePlayerBullet_Create(float startX, float startY)
{
	if (g_BulletFireCount >= MAX_BULLETS)
	{
		return;
	}

	Bullet& r = g_Bullets[g_BulletFireCount];
	r.posX = startX;
	r.posY = startY - BULLET_HEIGHT * 0.5f;
	r.anim_frame = 0;
	r.anim_timer = 0.0f;
	r.isDestroy = false;
	g_BulletFireCount++;
}

void GamePlayerBullet_Update(float delta_time)
{
	// Update bullet positions
	for (int i = 0; i < g_BulletFireCount; i++)
	{
		Bullet& b = g_Bullets[i];

		// Move right
		b.posX += BULLET_SPEED * delta_time;

		// Advance animation frame
		b.anim_timer += delta_time;
		if (b.anim_timer >= BULLET_FRAME_TIME)
		{
			b.anim_timer -= BULLET_FRAME_TIME;
			b.anim_frame = (b.anim_frame + 1) % BULLET_FRAME_MAX;
		}

		// Bullet destruction if it goes off screen
		if (g_Bullets[i].posX > SCREEN_WIDTH)
		{
			g_Bullets[i].isDestroy = true;
		}
	}
}

void GamePlayerBullet_Draw()
{
	for (int i = 0; i < g_BulletFireCount; i++)
	{
		const Bullet& b = g_Bullets[i];

		int col = b.anim_frame % BULLET_COLS; 
		int row = b.anim_frame / BULLET_COLS;

		float tex_x = static_cast<float>(col * BULLET_CELL_W);
		float tex_y = static_cast<float>(row * BULLET_CELL_H);

		Sprite_Draw(
			g_bullet_texture_ID,
			b.posX, b.posY,
			BULLET_WIDTH, BULLET_HEIGHT,
			tex_x, tex_y,
			BULLET_CELL_W, BULLET_CELL_H
		);
	}

#ifdef _DEBUG
	for (int i = 0; i < g_BulletFireCount; i++)
	{
		CollisionCircle cc = GamePlayerBullet_GetCollisionCircle(i);
		Collision_Debug_Draw(cc, { 1.0f, 0.0f, 0.0f });
	}
#endif
}

int GamePlayerBullet_GetActiveCount()
{
	return g_BulletFireCount;
}

void GamePlayerBullet_Destroy(int bulletIndex)
{
	if (bulletIndex < 0 || bulletIndex >= g_BulletFireCount)
	{
		return;
	}

	g_Bullets[bulletIndex].isDestroy = true;
}

void GamePlayerBullet_CleanUp()
{
	for (int i = g_BulletFireCount - 1; i >= 0; --i)
	{
		if (g_Bullets[i].isDestroy)
		{
			g_Bullets[i] = g_Bullets[g_BulletFireCount - 1];
			g_BulletFireCount--;
		}
	}
}

CollisionCircle GamePlayerBullet_GetCollisionCircle(int bulletIndex)
{
	if (bulletIndex < 0 || bulletIndex >= MAX_BULLETS)
	{
		return { {0.0f, 0.0f }, 0.0f };
	}
	
	return
	{
		{
			g_Bullets[bulletIndex].posX + BULLET_WIDTH * 0.8f,
			g_Bullets[bulletIndex].posY + BULLET_HEIGHT * 0.5f,
		},

		BULLET_HEIGHT * 0.3f
	};
}


