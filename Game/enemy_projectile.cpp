/*============================================================================
Contents   :  [enemy_projectile.cpp]

Author     : Chin Qing You
LastUpdate : 2026/08/17
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>

#include "enemy_projectile.h"
#include "game_player.h"
#include "game_stage.h"
#include "draw_primitives.h"

using namespace DirectX;

static constexpr int   PROJECTILE_MAX = 128;
static constexpr int   TRAIL_MAX = 8;
static constexpr float PROJECTILE_LIFE = 4.0f;
static constexpr float PROJECTILE_SIZE = 9.0f;    
static constexpr float HIT_RADIUS = 8.0f;

static constexpr float KNOCKBACK_SPEED = 220.0f;
static constexpr float KNOCKBACK_TIME = 0.15f;

struct Projectile
{
	Vector2  pos;
	Vector2  vel;
	Vector2  trail[TRAIL_MAX];
	int      trail_count;
	float    life;
	int      damage;
	XMFLOAT3 color;
	bool     active;
};

static Projectile g_Projectiles[PROJECTILE_MAX]{};
static int        g_ActiveCount = 0;

void EnemyProjectile_Initialize()
{
	for (Projectile& p : g_Projectiles) { p.active = false; }
	g_ActiveCount = 0;
}

void EnemyProjectile_Finalize()
{
	for (Projectile& p : g_Projectiles) { p.active = false; }
	g_ActiveCount = 0;
}

int EnemyProjectile_GetActiveCount() { return g_ActiveCount; }

void EnemyProjectile_Fire(const Vector2& pos, const Vector2& dir,
	float speed, int damage, const XMFLOAT3& color)
{
	int slot = -1;
	for (int i = 0; i < PROJECTILE_MAX; i++)
	{
		if (!g_Projectiles[i].active) { slot = i; break; }
	}
	if (slot < 0) { return; }  

	Projectile& p = g_Projectiles[slot];
	p.pos = pos;
	p.vel = dir * speed;
	p.trail_count = 0;
	p.life = PROJECTILE_LIFE;
	p.damage = damage;
	p.color = color;
	p.active = true;
	g_ActiveCount++;
}

static void RecordTrail(Projectile& p)
{
	if (p.trail_count < TRAIL_MAX)
	{
		p.trail[p.trail_count++] = p.pos;
		return;
	}
	for (int i = 0; i < TRAIL_MAX - 1; i++) { p.trail[i] = p.trail[i + 1]; }
	p.trail[TRAIL_MAX - 1] = p.pos;
}

void EnemyProjectile_Update(float delta_time)
{
	const Vector2 player = GamePlayer_GetPos();

	for (Projectile& p : g_Projectiles)
	{
		if (!p.active) { continue; }

		p.life -= delta_time;
		if (p.life <= 0.0f)
		{
			p.active = false;
			g_ActiveCount--;
			continue;
		}

		p.pos += p.vel * delta_time;
		RecordTrail(p);

		// --- wall ---
		if (GameStage_IsSolidAtWorld(p.pos.x, p.pos.y))
		{
			p.active = false;
			g_ActiveCount--;
			continue;
		}

		const float reach = HIT_RADIUS + 26.0f;   
		if ((player - p.pos).LengthSq() < reach * reach)
		{
			PlayerHit hit;
			hit.damage = p.damage;
			hit.knockback_speed = KNOCKBACK_SPEED;
			hit.knockback_time = KNOCKBACK_TIME;
			hit.direction = Vector2_Normalize(p.vel);

			GamePlayer_TakeHit(hit);

			p.active = false;
			g_ActiveCount--;
		}
	}
}

// ============================================================================
// Draw 
// ============================================================================
void EnemyProjectile_Draw()
{
	for (const Projectile& p : g_Projectiles)
	{
		if (!p.active) { continue; }

		if (p.trail_count >= 2)
		{
			DrawPrim_Trail(p.trail, p.trail_count, PROJECTILE_SIZE * 0.45f,
				p.color, 0.55f, 0.0f);
		}

		const Vector2 fwd = Vector2_Normalize(p.vel);
		const Vector2 side = { -fwd.y, fwd.x };        // rotate 90 deg

		const Vector2 tip = p.pos + fwd * PROJECTILE_SIZE;
		const Vector2 baseL = p.pos - fwd * (PROJECTILE_SIZE * 0.7f) + side * (PROJECTILE_SIZE * 0.7f);
		const Vector2 baseR = p.pos - fwd * (PROJECTILE_SIZE * 0.7f) - side * (PROJECTILE_SIZE * 0.7f);

		DrawPrim_Line(tip, baseL, 2.0f, p.color, 0.95f);
		DrawPrim_Line(tip, baseR, 2.0f, p.color, 0.95f);
		DrawPrim_Line(baseL, baseR, 2.0f, p.color, 0.95f);

		DrawPrim_Circle(p.pos, PROJECTILE_SIZE * 0.35f, p.color, 0.75f);
	}
}