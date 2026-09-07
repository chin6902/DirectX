/*============================================================================
Contents   :  [enemy_spawner.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/19
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>
#include <random>

#include "enemy_spawner.h"
#include "game_enemy.h"
#include "game_boss.h"
#include "game_player.h"
#include "game_stage.h"
#include "vector2.h"
#include "config.h"

static constexpr float SPAWN_RADIUS = 1000.0f;   
static constexpr float FORWARD_BIAS = 0.55f;     
static constexpr float WAVE_REST = 2.5f;     
static constexpr float BOSS_DISTANCE = 420.0f;   

struct WaveDef
{
	int   chasers;
	int   orbiters;
	int   elites;
	int   boss;           //  1 = slime 1, 2 = slime 2, 3 = both
	float hp_scale;
	float spawn_spread;   
	float timeout;        
};

static constexpr WaveDef g_Waves[] =
{
	//  ch  orb  eli  boss   hp    spread  timeout
	{   10,   0,   0,   0,  0.5f,   4.0f,  40.0f },   // 1  chasers
	{   12,   2,   0,   0,  1.0f,   5.0f,  45.0f },   // 2  chasers + orbiters
	{    6,   8,   0,   0,  1.5f,   5.0f,  50.0f },   // 3  more orbiters
	{   20,   3,   0,   0,  2.0f,   6.0f,  55.0f },   // 4  crowd pressure
	{   10,  10,   0,   0,  2.5f,   6.0f,  60.0f },   // 5  both at once
	{   10,   6,   1,   0,  2.5f,   6.0f,  75.0f },   // 6  first ELITE
	{   10,  15,   2,   0,  3.0f,   7.0f,  75.0f },   // 7  2 elites
	{   30,  15,   1,   0,  3.5f,   8.0f,   0.0f },   // 8  everything
	{    0,   0,   0,   1,  1.0f,   0.5f,   0.0f },   // 9  BOSS 1
	{   10,   0,   0,   0,  3.5f,   6.0f,  20.0f },   // 10 rest
	{    0,   0,   0,   6,  1.0f,   2.0f,   0.0f },   // 11 2 BOSSES 
};

static constexpr int WAVE_COUNT = static_cast<int>(sizeof(g_Waves) / sizeof(g_Waves[0]));

enum WaveState
{
	WAVE_SPAWNING,
	WAVE_FIGHTING,
	WAVE_REST_STATE,
	WAVE_FINISHED,    
};

static float     g_Elapsed = 0.0f;
static int       g_WaveIndex = 0;
static WaveState g_State = WAVE_SPAWNING;
static float     g_RestTimer = 0.0f;
static float     g_SpawnTimer = 0.0f;
static float     g_WaveClock = 0.0f;

static int  g_ChasersLeft = 0;
static int  g_OrbitersLeft = 0;
static int  g_ElitesLeft = 0;
static int  g_BossLeft = 0;

static std::mt19937 g_Rng{ std::random_device{}() };

static float RandFloat(float lo, float hi)
{
	std::uniform_real_distribution<float> d(lo, hi);
	return d(g_Rng);
}

static Vector2 RingPosition(float radius)
{
	const Vector2 player = GamePlayer_GetPos();
	const float facing = Vector2_ToAngle(GamePlayer_GetAimDir());

	const float uniform = RandFloat(0.0f, 6.2831853f);
	const float ahead = facing + RandFloat(-1.2f, 1.2f);
	const float angle = (RandFloat(0.0f, 1.0f) < FORWARD_BIAS) ? ahead : uniform;

	return GameStage_ClampPosition(player + Vector2_FromAngle(angle) * radius, 40.0f);
}

static bool IsBossWave(int index)
{
	return g_Waves[index].boss != 0;
}

static void BeginWave(int index)
{
	const WaveDef& w = g_Waves[index];

	GameEnemy_SetHPScale(w.hp_scale);

	g_ChasersLeft = w.chasers;
	g_OrbitersLeft = w.orbiters;
	g_ElitesLeft = w.elites;
	g_BossLeft = w.boss;

	g_SpawnTimer = 0.0f;
	g_RestTimer = 0.0f;
	g_WaveClock = 0.0f;

	int boss_owed = 0;
	for (int v = 1; v <= BOSS_VARIANT_COUNT; v++) { if (g_BossLeft & (1 << (v - 1))) { boss_owed++; } }
	const int total = g_ChasersLeft + g_OrbitersLeft + g_ElitesLeft + boss_owed;
	g_State = (total > 0) ? WAVE_SPAWNING : WAVE_FIGHTING;
}

void EnemySpawner_Initialize()
{
	g_Elapsed = 0.0f;
	g_WaveIndex = 0;
	g_WaveClock = 0.0f;

	GameEnemy_SetHPScale(g_Waves[0].hp_scale);
	g_State = WAVE_REST_STATE;
	g_RestTimer = 1.5f;
}

void EnemySpawner_Finalize() {}

float EnemySpawner_GetElapsed() { return g_Elapsed; }
int   EnemySpawner_GetWaveNumber() { return g_WaveIndex + 1; }
int   EnemySpawner_GetWaveCount() { return WAVE_COUNT; }

float EnemySpawner_GetWaveTimeout()
{ 
	if (EnemySpawner_IsFinished()) { return 0.0f; }
	if (IsBossWave(g_WaveIndex)) { return 0.0f; }

	const float timeout = g_Waves[g_WaveIndex].timeout;
	if (timeout <= 0.0f) { return 0.0f; }

	if (g_State == WAVE_REST_STATE) { return timeout; }

	return std::max(0.0f, timeout - g_WaveClock);
}

bool  EnemySpawner_IsBossWave() { return IsBossWave(g_WaveIndex); }
bool  EnemySpawner_IsFinished() { return g_State == WAVE_FINISHED; }

static void SpawnOne()
{
	const int swarm_left = g_ChasersLeft + g_OrbitersLeft;

	if (swarm_left > 0)
	{
		const bool chaser = (RandFloat(0.0f, 1.0f) < static_cast<float>(g_ChasersLeft) / static_cast<float>(swarm_left));

		if (chaser && g_ChasersLeft > 0)
		{
			GameEnemy_Create(ENEMY_TYPE_CHASER, RingPosition(SPAWN_RADIUS));
			g_ChasersLeft--;
		}
		else if (g_OrbitersLeft > 0)
		{
			GameEnemy_Create(ENEMY_TYPE_ORBITER, RingPosition(SPAWN_RADIUS));
			g_OrbitersLeft--;
		}
		else
		{
			GameEnemy_Create(ENEMY_TYPE_CHASER, RingPosition(SPAWN_RADIUS));
			g_ChasersLeft--;
		}
		return;
	}

	if (g_ElitesLeft > 0)
	{
		GameEnemy_Create(ENEMY_TYPE_ELITE, RingPosition(SPAWN_RADIUS));
		g_ElitesLeft--;
		return;
	}

	if (g_BossLeft > 0)
	{
		for (int variant = 1; variant <= BOSS_VARIANT_COUNT; variant++)
		{
			const int bit = 1 << (variant - 1);
			if ((g_BossLeft & bit) == 0) { continue; }

			GameBoss_Spawn(RingPosition(BOSS_DISTANCE), variant,
				GameBoss_GetActiveCount() == 0);

			g_BossLeft &= ~bit;
			return;
		}

		g_BossLeft = 0;  
	}
}

static bool WaveOwesAnything()
{
	return (g_ChasersLeft + g_OrbitersLeft + g_ElitesLeft + g_BossLeft) > 0;
}

static void AdvanceWave()
{
	g_WaveIndex++;

	if (g_WaveIndex >= WAVE_COUNT)
	{
		g_State = WAVE_FINISHED;
		return;
	}

	g_State = WAVE_REST_STATE;
	g_RestTimer = WAVE_REST;
}

void EnemySpawner_Update(float delta_time)
{
	g_Elapsed += delta_time;
	g_WaveClock += delta_time;

	switch (g_State)
	{
	case WAVE_SPAWNING:
	{
		const WaveDef& w = g_Waves[g_WaveIndex];
		int boss_owed = 0;
		for (int v = 1; v <= BOSS_VARIANT_COUNT; v++) { if (w.boss & (1 << (v - 1))) { boss_owed++; } }
		const int total = w.chasers + w.orbiters + w.elites + boss_owed;
		const float gap = (total > 1) ? (w.spawn_spread / total) : 0.0f;

		g_SpawnTimer += delta_time;

		if (gap <= 0.0f)
		{
			while (WaveOwesAnything()) { SpawnOne(); }
		}
		else
		{
			while (g_SpawnTimer >= gap && WaveOwesAnything())
			{
				g_SpawnTimer -= gap;
				SpawnOne();
			}
		}

		if (!WaveOwesAnything())
		{
			g_State = WAVE_FIGHTING;
		}
		break;
	}

	case WAVE_FIGHTING:
		if (IsBossWave(g_WaveIndex))
		{
			if (!GameBoss_AnyActive()) { AdvanceWave(); }
		}
		else if (GameEnemy_GetActiveCount() == 0 || (g_Waves[g_WaveIndex].timeout > 0.0f && g_WaveClock >= g_Waves[g_WaveIndex].timeout))
		{
			AdvanceWave();
		}
		break;

	case WAVE_REST_STATE:
		g_RestTimer -= delta_time;
		if (g_RestTimer <= 0.0f) { BeginWave(g_WaveIndex); }
		break;

	case WAVE_FINISHED:
		break;
	}
}