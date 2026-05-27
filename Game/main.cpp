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

static constexpr char WINDOW_CLASS[] = "GameWindow";
static constexpr char TITLE[] = "Game";

HDC hdc;
PAINTSTRUCT ps;
RECT rc;
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
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

	HWND hWnd = CreateWindow(WINDOW_CLASS, TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

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
			"確認", MB_OKCANCEL | MB_DEFBUTTON2) == IDOK) 
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
