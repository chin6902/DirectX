/*============================================================================
Contents   :  [fade.cpp]

Author     : Chin Qing You
LastUpdate : 2026/07/10
-----------------------------------------------------------------------------

============================================================================*/
#include <DirectXMath.h>
#include <utility>

#include "fade.h"
#include "texture.h"
#include "sprite.h"
#include "config.h"

using namespace DirectX;

static float g_AccumulatedTime = 0.0f;
static int g_TextureWhiteId = -1;

static float g_FadeTime = 0.0f;
static XMFLOAT4 g_FadeColor = { 0.0f, 0.0f, 0.0f, 0.0f };
static FadeType g_FadeType{};
static SpriteDrawParams g_FadeParams;

void Fade_Initialize()
{
	g_TextureWhiteId = Texture_Load(L"assets/textures/white.png");

	g_AccumulatedTime = 0.0f;
	g_FadeTime = 0.0f;
	g_FadeColor.w = 0.0f;
}

void Fade_Finalize()
{
	Texture_Release(g_TextureWhiteId);
}

void Fade_Update(float delta_time)
{
	if (g_FadeTime <= 0.0f)
	{
		return;
	}

	g_AccumulatedTime += delta_time;

	float alpha = std::min(1.0f, g_AccumulatedTime / g_FadeTime);

	if (g_FadeType == FADE_IN)
	{
		g_FadeColor.w = 1.0f - alpha;
	}
	else if (g_FadeType == FADE_OUT)
	{
		g_FadeColor.w = alpha;
	}
}

void Fade_Draw()
{
	if(g_FadeTime <= 0.0f || g_AccumulatedTime <= 0.0f || g_FadeColor.w <= 0.0f)
	{
		return;
	}

	g_FadeParams.color = { g_FadeColor.x, g_FadeColor.y, g_FadeColor.z };
	g_FadeParams.alpha = g_FadeColor.w;

	Sprite_Draw(
		g_TextureWhiteId,
		0.0f, 0.0f,
		static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT),
		g_FadeParams
	);
}

void Fade_Start(FadeType type, float fade_time, const DirectX::XMFLOAT4& fade_color)
{
	g_FadeType = type;
	g_FadeTime = fade_time;
	g_FadeColor = fade_color;
	g_AccumulatedTime = 0.0f;
}

bool Fade_IsFinished()
{
	return g_AccumulatedTime / g_FadeTime >= 1.0f;
}
