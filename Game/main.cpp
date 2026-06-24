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
#include <combaseapi.h>
#include <algorithm>
#include <iomanip>
#include <sstream>

#include "Window.h"
#include "application.h"
#include "debug_text.h"
#include "direct3d.h"
#include "config.h"
#include "system_timer.h"
#include "keyboard.h"
#include "mouse.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
	(void)CoInitializeEx(nullptr, COINIT_MULTITHREADED);
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

	// FPS表示 からの追加: 計測用変数
	double elapsed_time = 0.0;     // 1フレームの経過時間（秒）
	double time_accumulator = 0.0; // 経過時間の累積値
	int frame_counter = 0;         // フレーム数の累積カウント
	double fps = 0.0;              // 算出されたFPS値

	double fixed_time_accumulator = 0.0;
	static constexpr double FIXED_DELTA_TIME = 1.0 / 60.0;

	// 各システムの初期化
	if (Application_Initialize(hWnd))
	{
#ifdef _DEBUG
		hal::DebugText debug_text(
			Direct3D_GetDevice(),
			Direct3D_GetDeviceContext(),
			L"assets/textures/Sixtyfour-Regular_ascii_512.png",
			SCREEN_WIDTH, SCREEN_HEIGHT
		);
#endif

		// FPS表示 からの追加: タイマーの初期化とスレッドアフィニティ設定
		SystemTimer_Initialize();
		LimitThreadAffinityToCurrentProc(); // 実行スレッドを固定してタイマーの誤差を防ぐ
		SystemTimer_Start();

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
				// FPS表示 からの追加: 前回のフレームからの経過時間（秒）を取得
				elapsed_time = SystemTimer_GetElapsedTime(); 

				// 1フレーム当たりの最大経過時間を0.1秒(100ms)に制限する
				if (elapsed_time > 0.1)
				{
					elapsed_time = 0.1;
				}

				time_accumulator += elapsed_time;
				frame_counter++;

				// 1.0秒経過したら、その間のフレーム数からFPSを確定して累積をリセット
				if (time_accumulator >= 1.0)
				{ 
					fps = frame_counter / time_accumulator; 
					frame_counter = 0; 
					time_accumulator = 0.0; 
				}

				// 固定フレームのための経過時間の累積
				fixed_time_accumulator += elapsed_time;

				int update_count = 0;

				// 1/60秒経ったら実行する
				while (fixed_time_accumulator >= FIXED_DELTA_TIME && update_count < 5)
				{
					Application_FixedUpdate();
					fixed_time_accumulator -= FIXED_DELTA_TIME;
					update_count++;
				}

				Direct3D_Begin();

				Application_Update((float)elapsed_time);

				Application_Draw();

#ifdef _DEBUG
				debug_text.Clear();
				// FPS表示 からの追加: 計測されたFPSとフレーム経過時間(ms)をstd::stringstreamで整形
				std::stringstream ss; ss << "FPS: " << std::fixed << std::setprecision(2) << fps << " (" << std::fixed << std::setprecision(2) << (elapsed_time * 1000.0) << " ms)";
				debug_text.SetText(ss.str().c_str());
				
				debug_text.Draw();
#endif

				Direct3D_Flip();
			}

		} while (msg.message != WM_QUIT);
	}

	// 各システムの終了処理
	Application_Finalize();

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
	case WM_DESTROY: // ウィンドウの破棄メッセージ
		PostQuitMessage(0); // WM_QUITメッセージの送信
		break;
    case WM_ACTIVATEAPP:
		Keyboard_ProcessMessage(message, wParam, lParam);
		Mouse_ProcessMessage(message, wParam, lParam);
		break;
    case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			SendMessage(hWnd, WM_CLOSE, 0, 0); // WM_CLOSEメッセージの送信
		}
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        Keyboard_ProcessMessage(message, wParam, lParam);
        break;
    case WM_INPUT:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEHOVER:
        Mouse_ProcessMessage(message, wParam, lParam);
        break; 
	default:
		// 通常のメッセージ処理はこの関数に任せる
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
