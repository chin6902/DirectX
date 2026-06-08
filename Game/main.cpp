/*============================================================================
Contents   :  [main.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/01
-----------------------------------------------------------------------------
Windows program
============================================================================*/
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Window.h"
#include "direct3d.h"
#include "shader.h"
#include "polygon.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// ウィンドウの作成
	HWND hWnd{ CreateGameWindow(hInstance, WndProc) };

	if (hWnd == nullptr)
	{
		MessageBox(
			nullptr,
			"Failed to create game window.",
			"Error",
			MB_OK | MB_ICONERROR
		);

		return -1;
	}
	
	MSG msg{};

	// Direct3Dの初期化
	if (Direct3DInitialize(hWnd))
	{
		// shaderの初期化
		Shader_Initialize(Direct3D_GetDevice(), Direct3D_GetDeviceContext());

		// polygonの初期化
		Polygon_Initialize();

		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);

		do
		{
			// Window message loop
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			// Game loop
			else
			{
				Direct3D_Begin();
				Polygon_Draw();
				Direct3D_Flip();
			}

		} while (msg.message != WM_QUIT);
	}

	// polygonの終了処理
	Polygon_Finalize();

	// shaderの終了処理
	Shader_Finalize();

	// Direct3Dの終了処理
	Direct3DFinalize();

	return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE: // ウィンドウを閉じるメッセージ
		if (MessageBox(hWnd, "本当に終了してよろしいですか？",
			"確認", MB_ICONEXCLAMATION | MB_OKCANCEL | MB_DEFBUTTON2) == IDOK) 
		{
			DestroyWindow(hWnd); // 指定のウィンドウにWM_DESTROYメッセージを送る
		}
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) 
		{
			SendMessage(hWnd, WM_CLOSE, 0, 0); // WM_CLOSEメッセージの送信
		}
		break;
	case WM_DESTROY: // ウィンドウの破棄メッセージ
		PostQuitMessage(0); // WM_QUITメッセージの送信
		break;
	default:
		// 通常のメッセージ処理はこの関数に任せる
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
