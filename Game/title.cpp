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
#include "input_keyboard.h"
#include "Audio.h"

static int g_titleBG_texture = -1;
static int g_titleButton_texture = -1;
static constexpr float BUTTON_WIDTH = 300.0f;
static constexpr float BUTTON_HEIGHT = 100.0f;

SpriteDrawParams g_buttonParams;
static float g_buttonAlpha = 1.0f;
static int g_pulseDirection = -1;  
static constexpr float MIN_ALPHA = 0.3f;
static constexpr float MAX_ALPHA = 1.0f;
float pulse_speed = 0.6f;

void ButtonPulse(float delta_time);

void Title_Initialize()
{
	g_titleBG_texture = Texture_Load(L"assets/textures/title.png");
	g_titleButton_texture = Texture_Load(L"assets/textures/title_button.png");	
}

void Title_Finalize()
{
	Texture_Release(g_titleBG_texture);
	Texture_Release(g_titleButton_texture);
}

void Title_Update(float delta_time)
{
	if (InputKeyboard_IsTrigger(KK_ENTER))
	{
		Scene_SetNextScene(SCENE_GAME);
	}

	ButtonPulse(delta_time);
}

void Title_Draw()
{
	Sprite_Draw(g_titleBG_texture, 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);

	Sprite_Draw(
		g_titleButton_texture,
		SCREEN_WIDTH / 2.0f - BUTTON_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - BUTTON_HEIGHT / 2.0f + 200.0f,
		BUTTON_WIDTH, BUTTON_HEIGHT,
		g_buttonParams
	);
}

void ButtonPulse(float delta_time)
{
	g_buttonAlpha += g_pulseDirection * pulse_speed * delta_time;
	if (g_buttonAlpha >= MAX_ALPHA)
	{
		g_buttonAlpha = MAX_ALPHA;
		g_pulseDirection = -1;
		pulse_speed = 0.4f;
	}
	else if (g_buttonAlpha <= MIN_ALPHA)
	{
		g_buttonAlpha = MIN_ALPHA;
		g_pulseDirection = 1;
		pulse_speed = 0.6f;
	}

	g_buttonParams.alpha = g_buttonAlpha;
}
