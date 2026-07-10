/*============================================================================
Contents   :  [title.cpp]

Author     : Chin Qing You
LastUpdate : 2026/07/09
-----------------------------------------------------------------------------

============================================================================*/
#include "title.h"
#include "texture.h"
#include "sprite.h"
#include "config.h"
#include "scene.h"
#include "fade.h"

#include "input_keyboard.h"
#include "Audio.h"

static int g_TitleBG_texture = -1;
static int g_TitleButton_texture = -1;
static constexpr float BUTTON_WIDTH = 300.0f;
static constexpr float BUTTON_HEIGHT = 100.0f;

SpriteDrawParams g_buttonParams;
static float g_ButtonAlpha = 1.0f;
static int g_PulseDirection = -1;  
static constexpr float MIN_ALPHA = 0.3f;
static constexpr float MAX_ALPHA = 1.0f;
float pulse_speed = 0.6f;

static bool g_IsChangeScene = false;

void ButtonPulse(float delta_time);

void Title_Initialize()
{
	g_TitleBG_texture = Texture_Load(L"assets/textures/title.png");
	g_TitleButton_texture = Texture_Load(L"assets/textures/title_button.png");	

	g_IsChangeScene = false;
}

void Title_Finalize()
{
	Texture_Release(g_TitleBG_texture);
	Texture_Release(g_TitleButton_texture);
}

void Title_Update(float delta_time)
{
	if (!g_IsChangeScene)
	{
		if (InputKeyboard_IsTrigger(KK_ENTER))
		{
			Fade_Start(FADE_OUT, 1.0f, { 0.0f, 0.0f, 0.0f, 1.0f });
			g_IsChangeScene = true;
		}

		ButtonPulse(delta_time);
	}
	else
	{
		if (Fade_IsFinished())
		{
			Scene_SetNextScene(SCENE_GAME);
		}
	}
}

void Title_Draw()
{
	Sprite_Draw(g_TitleBG_texture, 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT));

	Sprite_Draw(
		g_TitleButton_texture,
		static_cast<float>(SCREEN_WIDTH) / 2.0f - BUTTON_WIDTH / 2.0f, static_cast<float>(SCREEN_HEIGHT) / 2.0f - BUTTON_HEIGHT / 2.0f + 200.0f,
		BUTTON_WIDTH, BUTTON_HEIGHT,
		g_buttonParams
	);
}

void ButtonPulse(float delta_time)
{
	g_ButtonAlpha += g_PulseDirection * pulse_speed * delta_time;
	if (g_ButtonAlpha >= MAX_ALPHA)
	{
		g_ButtonAlpha = MAX_ALPHA;
		g_PulseDirection = -1;
		pulse_speed = 0.4f;
	}
	else if (g_ButtonAlpha <= MIN_ALPHA)
	{
		g_ButtonAlpha = MIN_ALPHA;
		g_PulseDirection = 1;
		pulse_speed = 0.6f;
	}

	g_buttonParams.alpha = g_ButtonAlpha;
}
