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

static float SpawnX = SCREEN_WIDTH + 50.0f; 

static void SpawnEnemy();
static float timer = 0.0f;
static constexpr float spawn_interval = 5.0f; // Spawn an enemy every 5 seconds

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
