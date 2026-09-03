/*============================================================================
Contents   :  [camera.cpp]

Author     : Chin Qing You
LastUpdate : 2026/07/21
-----------------------------------------------------------------------------

============================================================================*/
#include <cmath>
#include <algorithm>

#include "camera.h"
#include "config.h"
#include "game_player.h"
#include "vector2.h"
#include "game_stage.h"

static Vector2 g_Camera;
static constexpr float CAMERA_LERP_SPEED = 8.0f;

static float g_CameraX = 0.0f;
static float g_CameraY = 0.0f;

static float g_ShakeStrength = 0.0f;
static float g_ShakeTimer = 0.0f;
static float g_ShakeMax = 0.0f;

void Camera_Initialize()
{
	g_CameraX = GamePlayer_GetPosX() - SCREEN_WIDTH * 0.5f;
	g_CameraY = GamePlayer_GetPosY() - SCREEN_HEIGHT * 0.5f;
}

void Camera_Update(float delta_time)
{
	const Vector2 target{
		GamePlayer_GetPosX() - SCREEN_WIDTH * 0.5f,
		GamePlayer_GetPosY() - SCREEN_HEIGHT * 0.5f
	};

	const float t = 1.0f - expf(-CAMERA_LERP_SPEED * delta_time);
	g_Camera = Vector2_Lerp(g_Camera, target, t);

	g_Camera.x = std::clamp(g_Camera.x, 0.0f, GameStage_GetWidth() - SCREEN_WIDTH);
	g_Camera.y = std::clamp(g_Camera.y, 0.0f, GameStage_GetHeight() - SCREEN_HEIGHT);

	if (g_ShakeTimer > 0.0f)
	{
		g_ShakeTimer -= delta_time;

		const float fade = (g_ShakeMax > 0.0f) ? (g_ShakeTimer / g_ShakeMax) : 0.0f;
		const float amt = g_ShakeStrength * std::max(0.0f, fade);

		g_Camera.x += ((rand() % 200) - 100) * 0.01f * amt;
		g_Camera.y += ((rand() % 200) - 100) * 0.01f * amt;

		if (g_ShakeTimer <= 0.0f) { g_ShakeStrength = 0.0f; }
	}
}

void Camera_Shake(float strength, float duration)
{
	if (strength <= g_ShakeStrength && g_ShakeTimer > 0.0f) { return; }

	g_ShakeStrength = strength;
	g_ShakeTimer = duration;
	g_ShakeMax = duration;
}

float Camera_GetX()
{
	return g_Camera.x;
}

float Camera_GetY()
{
	return g_Camera.y;
}

float Camera_WorldToScreenX(float world_x)
{
	return world_x - g_Camera.x;
}

float Camera_WorldToScreenY(float world_y)
{
	return world_y - g_Camera.y;
}

float Camera_ScreenToWorldX(float screen_x)
{ 
	return screen_x + g_Camera.x; 
}

float Camera_ScreenToWorldY(float screen_y)
{
	return screen_y + g_Camera.y; 
}
