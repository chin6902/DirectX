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
#include "game.h"

#include "input_keyboard.h"
#include "input_mouse.h"
#include "input_xinput.h"

static int g_mouseX = 0;
static int g_mouseY = 0;

bool Application_Initialize(HWND hWnd)
{
	if (!Direct3DInitialize(hWnd))
	{
		return false;
	}

	// 各システムの初期化
	InputKeyboard_Initialize();
	InputMouse_Initialize(hWnd);
	InputXInput_Initialize();

	Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());
	Sprite_Initialize();

	Game_Initialize();

	return true;
}

void Application_Finalize()
{
	// 各システムの終了処理
	Game_Finalize();

	Sprite_Finalize();
	Texture_Finalize();
	Shader_Finalize();

	InputMouse_Finalize();

	Direct3DFinalize();
}

void Application_Update(float delta_time)
{
	// ゲームの更新処理
	InputKeyboard_Update(delta_time);
	InputMouse_Update();
	InputXInput_Update(delta_time);
	/*
		g_mouseX = InputMouse_GetX();
		g_mouseY = InputMouse_GetY();
	*/

	Game_Update(delta_time);
}

void Application_FixedUpdate()
{

}

void Application_Draw()
{
	Game_Draw();
}

