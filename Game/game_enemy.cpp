/*============================================================================
Contents   :  [game_enemy.cpp]

Author     : Chin Qing You
LastUpdate : 2026/06/29
-----------------------------------------------------------------------------

============================================================================*/
#include <math.h>
#include <random>

#include "game_enemy.h"
#include "config.h"
#include "texture.h"
#include "sprite.h"
#include "collision.h"
#include "collision_debug.h"
#include "game_impact.h"

struct EnemySpriteInfo
{
	int textureID;
	float width;
	float height;
	const wchar_t* textureName;
	CollisionCircle collisionCircle;
	ExplosionType explosionType;
};

enum EnemyState
{
	EnemyState_None,
	EnemyState_Spawn,
	EnemyState_Move,
	EnemyState_Dead,
};

struct Enemy
{
	float posX;
	float posY;
	float phaseOffset; 

	EnemyType type;
	EnemyState state;

	bool isDead;
};

static const EnemySpriteInfo g_EnemySpriteData[2] = 
{
	// EnemyType_Normal
	{
		.textureID = -1, 
		.width = 96.0f,
		.height = 64.0f,
		.textureName = L"assets/textures/Enemy.png",
		.collisionCircle = { { 48.0f, 32.0f }, 32.0f }, // Center at (48,32) with radius 32 //modify so that it can be calculated automatically based on width and height
		.explosionType = ExplosionType_Large,
	},
	// EnemyType_Fast
	{
		.textureID = -1, 
		.width = 64.0f,
		.height = 48.0f,
		.textureName = L"assets/textures/Enemy_type2.png",
		.collisionCircle = { { 32.0f, 24.0f }, 24.0f }, // Center at (32,24) with radius 24
		.explosionType = ExplosionType_Small,
	}
};

static EnemySpriteInfo g_EnemySpriteInfo[2]{};

static constexpr int MAX_ENEMIES = 100;
static Enemy g_Enemies[MAX_ENEMIES]{};
static int g_EnemyCount = 0;
static float g_enemy_speed = -100.0f;

void NormalMovement(Enemy& e, float delta_time);
void FastMovement(Enemy& e, float delta_time);

void GameEnemy_Initialize()
{
	for (int i = 0; i < 2; i++)
	{
		g_EnemySpriteInfo[i] = g_EnemySpriteData[i];
		g_EnemySpriteInfo[i].textureID = Texture_Load(g_EnemySpriteData[i].textureName);
	}
}

void GameEnemy_Finalize()
{
	Texture_Release(g_EnemySpriteInfo[EnemyType_Normal].textureID);
	Texture_Release(g_EnemySpriteInfo[EnemyType_Fast].textureID);
}

void GameEnemy_Create(EnemyType type, float startX, float startY)
{
	if (g_EnemyCount >= MAX_ENEMIES)
	{
		return;
	}

	Enemy& r = g_Enemies[g_EnemyCount];
	r.posX = startX;
	r.posY = startY;
	r.type = type;
	r.isDead = false;

	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dis(0.0f, 6.28f);  // 0 to 2ƒÎ
	r.phaseOffset = dis(gen);

	r.state = EnemyState_Spawn;

	g_EnemyCount++;
}

void GameEnemy_Update(float delta_time)
{
	// Update enemy positions
	for (int i = 0; i < g_EnemyCount; i++)
	{
		Enemy& e = g_Enemies[i];

		switch (g_Enemies[i].type)
		{
		case EnemyType_Normal:
			NormalMovement(e, delta_time);
			break;
		case EnemyType_Fast:
			FastMovement(e, delta_time);
			break;
		default:
			NormalMovement(e, delta_time);
			break;
		}
	}
}

void GameEnemy_Draw()
{
	SpriteDrawParams p;
	p.flip_x = true;

	for (int i = 0; i < g_EnemyCount; i++)
	{
		const Enemy& e = g_Enemies[i];
		const EnemySpriteInfo& info = g_EnemySpriteInfo[e.type];

		Sprite_Draw(
			info.textureID,
			e.posX, e.posY,
			info.width, info.height,
			p
		);
	}

#ifdef _DEBUG
	for (int i = 0; i < g_EnemyCount; i++)
	{
		CollisionCircle cc = GameEnemy_GetCollisionCircle(i);
		Collision_Debug_Draw(cc, { 1.0f, 1.0f, 0.0f });
	}
#endif
}

void NormalMovement(Enemy& e, float delta_time)
{
	g_enemy_speed = -100.0f;
	e.posX += g_enemy_speed * delta_time;

	if (e.posX < 0 - g_EnemySpriteInfo[e.type].width)
	{
		e.isDead = true;
	}
}

void FastMovement(Enemy& e, float delta_time)
{
	switch (e.state)
	{
	case EnemyState_Spawn:
	{
		static float time = 0;
		g_enemy_speed = -150.0f;
		e.posX += g_enemy_speed * delta_time;
		e.posY += 50.0f * sinf(e.posX * 0.05f + e.phaseOffset) * delta_time;
		e.phaseOffset += delta_time;

		if (e.posX < SCREEN_WIDTH * 0.5)
		{
			e.state = EnemyState_Move;
		}
		break;
	}
	case EnemyState_Move:
		NormalMovement(e, delta_time);

		if (e.posX < 0 - g_EnemySpriteInfo[e.type].width)
		{
			e.isDead = true;
		}
		break;
	}
}

ExplosionType GameEnemy_GetExplosionType(int enemyIndex)
{
	if (enemyIndex < 0 || enemyIndex >= g_EnemyCount)
	{
		return ExplosionType_Large;   // safe fallback
	}

	return g_EnemySpriteInfo[g_Enemies[enemyIndex].type].explosionType;
}

int GameEnemy_GetActiveCount()
{
	return g_EnemyCount;
}

void GameEnemy_Destroy(int enemyIndex)
{
	if (enemyIndex < 0 || enemyIndex >= g_EnemyCount)
	{
		return;
	}

	g_Enemies[enemyIndex].isDead = true;
}

void GameEnemy_CleanUp()
{
	for (int i = g_EnemyCount - 1; i >= 0; --i)
	{
		if (g_Enemies[i].isDead)
		{
			g_Enemies[i] = g_Enemies[g_EnemyCount - 1];
			g_EnemyCount--;
		}
	}
}

CollisionCircle GameEnemy_GetCollisionCircle(int enemyIndex)
{
	//if not debug build do not check for index out of bounds(+ points)
	if (enemyIndex < 0 || enemyIndex >= g_EnemyCount)
	{
		return { {0.0f, 0.0f}, 0.0f }; // Return an empty circle if index is invalid
	}

	CollisionCircle cc = g_EnemySpriteInfo[g_Enemies[enemyIndex].type].collisionCircle;

	cc.position.x += g_Enemies[enemyIndex].posX;
	cc.position.y += g_Enemies[enemyIndex].posY;

	return cc;
}


