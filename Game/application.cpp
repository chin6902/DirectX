/*============================================================================
Contents   :  [application.cpp]

Author     : Chin Qing You
LastUpdate : 2026/06/15
-----------------------------------------------------------------------------

============================================================================*/
#include "application.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"
#include "sprite.h"
#include "flipbook_animation.h"
#include "scene.h"
#include "fade.h"

#include "input_keyboard.h"
#include "input_mouse.h"
#include "input_xinput.h"
#include "Audio.h"

static int g_mouseX = 0;
static int g_mouseY = 0;

bool Application_Initialize(HWND hWnd)
{
	if (!Direct3DInitialize(hWnd))
	{
		return false;
	}

	InitAudio();

	// 各システムの初期化
	InputKeyboard_Initialize();
	InputMouse_Initialize(hWnd);
	InputXInput_Initialize();

	Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
	Sprite_Initialize();
	FlipBookAnimation_Initialize();

	Scene_Initialize();
	Fade_Initialize();

	return true;
}

void Application_Finalize()
{
	// 各システムの終了処理
	Fade_Finalize();
	Scene_Finalize();
	FlipBookAnimation_Finalize();
	Sprite_Finalize();
	Texture_Finalize();
	Shader_Finalize();

	InputMouse_Finalize();
	UninitAudio();
	Direct3DFinalize();
}

void Application_Update(float delta_time)
{
	Scene_Change();

	// ゲームの更新処理
	InputKeyboard_Update(delta_time);
	InputMouse_Update();
	/*
		InputXInput_Update(delta_time);
		g_mouseX = InputMouse_GetX();
		g_mouseY = InputMouse_GetY();
	*/

	Scene_Update(delta_time);
	Fade_Update(delta_time);
}

void Application_FixedUpdate()
{
}

void Application_Draw()
{
	Scene_Draw();
	Fade_Draw();
}

