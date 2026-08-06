/*============================================================================
Contents   :  [enemy_spawner.cpp]

Author     : Chin Qing You
LastUpdate : 2026/07/01
-----------------------------------------------------------------------------

============================================================================*/
#include <random>

#include "enemy_spawner.h"
#include "game_enemy.h"
#include "config.h"

struct SpawnData
{
	EnemyType type;
	float spawnX;
	float spawnY;
	float spawnTime;
};

// use text file to store spawn data for more flexibility
static constexpr SpawnData spawnData[3][1200] =
{
	{
		{ EnemyType_Normal, SCREEN_WIDTH + 50.0f, 100.0f, 0.0f },
		{ EnemyType_Fast, SCREEN_WIDTH + 50.0f, 200.0f, 1.0f },
		{ EnemyType_Normal, SCREEN_WIDTH + 50.0f, 300.0f, 2.0f },
		{ EnemyType_Fast, SCREEN_WIDTH + 50.0f, 400.0f, 3.0f },
		{ EnemyType_Invalid, 0.0f, 0.0f, -1.0f }
	},

	{
		{ EnemyType_Normal, SCREEN_WIDTH + 50.0f, 100.0f, 0.0f },
		{ EnemyType_Fast, SCREEN_WIDTH + 50.0f, 200.0f, 1.0f },
		{ EnemyType_Normal, SCREEN_WIDTH + 50.0f, 300.0f, 2.0f },
		{ EnemyType_Fast, SCREEN_WIDTH + 50.0f, 400.0f, 3.0f },
		{ EnemyType_Invalid, 0.0f, 0.0f, -1.0f }
	},

	{
		{ EnemyType_Normal, SCREEN_WIDTH + 50.0f, 100.0f, 0.0f },
		{ EnemyType_Fast, SCREEN_WIDTH + 50.0f, 200.0f, 1.0f },
		{ EnemyType_Normal, SCREEN_WIDTH + 50.0f, 300.0f, 2.0f },
		{ EnemyType_Fast, SCREEN_WIDTH + 50.0f, 400.0f, 3.0f },
		{ EnemyType_Invalid, 0.0f, 0.0f, -1.0f }
	}
};

static int g_StageNumber = 0;
static int g_CurrentIndex = 0; // where in list of spawn data we are currently at
static float acumulatedTime = 0.0f; 

static float SpawnX = SCREEN_WIDTH + 50.0f; 

static void SpawnEnemy();
static float timer = 0.0f;
static constexpr float spawn_interval = 2.0f; 

void EnemySpawner_Initialize()
{
	SpawnEnemy();	
}

void EnemySpawner_Finalize()
{
}

void EnemySpawner_Update(float delta_time)
{
	timer += delta_time;
	if (timer >= spawn_interval)
	{
		SpawnEnemy();
		timer = 0.0f;
	}
}

static void SpawnEnemy()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> disY(0.0f, SCREEN_HEIGHT - 64.0f);
	static std::uniform_int_distribution<int> disType(0, 1);

	float spawnY = disY(gen);

	int randomType = disType(gen);
	EnemyType type{};

	switch(randomType)
	{
		case 0:
			type = EnemyType_Normal;
			break;
		case 1:
			type = EnemyType_Fast;
			break;
		default:
			type = EnemyType_Normal;
			break;
	}

	GameEnemy_Create(type, SpawnX, spawnY);
}
