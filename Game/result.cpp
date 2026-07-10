/*============================================================================
Contents   :  [result.cpp]

Author     : Chin Qing You
LastUpdate : 2026/07/09
-----------------------------------------------------------------------------

============================================================================*/
#include "result.h"
#include "texture.h"
#include "sprite.h"
#include "config.h"
#include "input_keyboard.h"
#include "scene.h"
#include "fade.h"

static int g_result_texture_id = -1;
static bool g_IsChangeScene = false;

void Result_Initialize()
{
	g_result_texture_id = Texture_Load(L"assets/textures/result.png");

	Fade_Start(FADE_IN, 1.0f, { 0.0f, 0.0f, 0.0f, 0.0f });
	g_IsChangeScene = false;
}

void Result_Finalize()
{
	Texture_Release(g_result_texture_id);
}

void Result_Update(float delta_time)
{
	if (InputKeyboard_IsTrigger(KK_ENTER))
	{
		Scene_SetNextScene(SCENE_TITLE);
	}
}

void Result_Draw()
{
	Sprite_Draw(g_result_texture_id, 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT);
}
