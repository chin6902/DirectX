/*============================================================================
Contents   :  [main.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/05/27
-----------------------------------------------------------------------------
Windows program
============================================================================*/
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <algorithm>

#include "config.h"

using namespace std;

static constexpr char WINDOW_CLASS[] = "GameWindow";
static constexpr char TITLE[] = "Game";

HDC hdc;
PAINTSTRUCT ps;
RECT rc;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	WNDCLASSEX wcex
	{
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = WndProc,
		.hInstance = hInstance,
		.hIcon = LoadIcon(hInstance, IDI_APPLICATION),
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = (HBRUSH)(GetStockObject(BLACK_BRUSH)),
		.lpszClassName = WINDOW_CLASS,
		.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION),
	};

	RegisterClassEx(&wcex);

	// window size
	RECT window_rect = { .left = 0, .top = 0, .right = SCREEN_WIDTH, .bottom = SCREEN_HEIGHT };

	//window style
	constexpr DWORD WINDOW_STYLE = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

	AdjustWindowRect(&window_rect, WINDOW_STYLE, FALSE);

	// Calculate the actual window size after adjusting for the window style
	const int WINDOW_WIDTH { window_rect.right - window_rect.left };
	const int WINDOW_HEIGHT { window_rect.bottom - window_rect.top };

	// Get the main display desktop resolution
	int desktop_width = GetSystemMetrics(SM_CXSCREEN);
	int desktop_height = GetSystemMetrics(SM_CYSCREEN);

	// Center the window on the desktop
	int window_x = std::max((desktop_width - WINDOW_WIDTH) / 2, 0);
	int window_y = std::max((desktop_height - WINDOW_HEIGHT) / 2, 0);

	HWND hWnd = CreateWindow(
		WINDOW_CLASS, TITLE, WINDOW_STYLE,
		window_x, window_y,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		nullptr, nullptr, hInstance, nullptr
	);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg{};

	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
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
