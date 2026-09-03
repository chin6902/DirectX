/*============================================================================
Contents   :  [enemy_formation.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/18
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>

#include "enemy_formation.h"
#include "game_enemy.h"
#include "config.h"

static constexpr int   FORMATION_MAX = 8;
static constexpr float MEMBER_SPACING = 62.0f;
static constexpr float TRAVEL_PADDING = 400.0f; 

struct Formation
{
	int     id;            
	Vector2 origin;
	Vector2 vel;
	float   travel_left;
	bool    active;
};

static Formation g_Formations[FORMATION_MAX]{};
static int       g_NextId = 1;         

void EnemyFormation_Initialize()
{
	for (Formation& f : g_Formations) { f.active = false; f.id = 0; }
	g_NextId = 1;
}

void EnemyFormation_Finalize()
{
	EnemyFormation_ClearAll();
}

void EnemyFormation_ClearAll()
{
	for (Formation& f : g_Formations) { f.active = false; f.id = 0; }
}

int EnemyFormation_GetActiveCount()
{
	int n = 0;
	for (const Formation& f : g_Formations) { if (f.active) { n++; } }
	return n;
}

static Formation* Find(int id)
{
	if (id == 0) { return nullptr; }

	for (Formation& f : g_Formations)
	{
		if (f.active && f.id == id) { return &f; }
	}
	return nullptr;
}

Vector2 EnemyFormation_GetOrigin(int formation_id)
{
	const Formation* f = Find(formation_id);
	return (f != nullptr) ? f->origin : Vector2{ 0.0f, 0.0f };
}

bool EnemyFormation_IsActive(int formation_id)
{
	return Find(formation_id) != nullptr;
}

static int Create(const Vector2& origin, const Vector2& vel, float travel)
{
	for (Formation& f : g_Formations)
	{
		if (f.active) { continue; }

		f.id = g_NextId++;
		f.origin = origin;
		f.vel = vel;
		f.travel_left = travel;
		f.active = true;
		return f.id;
	}
	return 0;
}

void EnemyFormation_SpawnTriangle(const Vector2& origin, const Vector2& dir,
	float speed, int rows)
{
	const Vector2 fwd = Vector2_Normalize(dir);
	const Vector2 side = { -fwd.y, fwd.x };

	const float travel = static_cast<float>(SCREEN_WIDTH + SCREEN_HEIGHT) + TRAVEL_PADDING;
	const int   id = Create(origin, fwd * speed, travel);
	if (id == 0) { return; }

	for (int r = 0; r < rows; r++)
	{
		for (int i = 0; i <= r; i++)
		{
			const float lateral = (i - r * 0.5f) * MEMBER_SPACING;
			const Vector2 offset = fwd * (-MEMBER_SPACING * r) + side * lateral;

			GameEnemy_CreateInFormation(ENEMY_TYPE_DUMMY, id, offset);
		}
	}
}

void EnemyFormation_SpawnWall(const Vector2& origin, float dir_x, float speed)
{
	const float travel = static_cast<float>(SCREEN_WIDTH) + TRAVEL_PADDING * 2.0f;
	const int   id = Create(origin, Vector2{ (dir_x > 0.0f) ? 1.0f : -1.0f, 0.0f } * speed, travel);
	if (id == 0) { return; }

	const int count = static_cast<int>(SCREEN_HEIGHT / MEMBER_SPACING) + 2;

	for (int i = 0; i < count; i++)
	{
		const float y = (i - (count - 1) * 0.5f) * MEMBER_SPACING;
		GameEnemy_CreateInFormation(ENEMY_TYPE_DUMMY, id, { 0.0f, y });
	}
}

void EnemyFormation_Update(float delta_time)
{
	for (Formation& f : g_Formations)
	{
		if (!f.active) { continue; }

		const float step = f.vel.Length() * delta_time;
		f.origin += f.vel * delta_time;
		f.travel_left -= step;

		if (f.travel_left <= 0.0f || GameEnemy_CountInFormation(f.id) == 0)
		{
			f.active = false;
			f.id = 0;
		}
	}
}

void EnemyFormation_Draw()
{
}