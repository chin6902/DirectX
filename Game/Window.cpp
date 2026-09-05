/*============================================================================
Contents   :  [Window.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/01
-----------------------------------------------------------------------------

============================================================================*/
#include "Window.h"
#include "config.h"
#include "resource.h"

#include <algorithm>

namespace
{
	static constexpr char WINDOW_CLASS[] = "GameWindow";
	static constexpr char TITLE[] = "SpellForge";

	constexpr DWORD WINDOW_STYLE
	{
		WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX)
	};

	bool RegisterGameWindowClass(HINSTANCE hInstance, WNDPROC wndProc)
	{
		WNDCLASSEX wcex
		{
			.cbSize = sizeof(WNDCLASSEX),
			.style = CS_HREDRAW | CS_VREDRAW,
			.lpfnWndProc = wndProc,
			.hInstance = hInstance,
			.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAME_ICON)),
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)),
			.lpszClassName = WINDOW_CLASS,
			.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAME_ICON)),
		};

		if (RegisterClassEx(&wcex) == 0)
		{
			return false;
		}

		return true;
	}
}

HWND CreateGameWindow(HINSTANCE hInstance, WNDPROC wndProc)
{
	if (!RegisterGameWindowClass(hInstance, wndProc))
	{
		return nullptr;
	}

	RECT window_rect
	{
		.left = 0,
		.top = 0,
		.right = SCREEN_WIDTH,
		.bottom = SCREEN_HEIGHT
	};

	if (AdjustWindowRect(&window_rect, WINDOW_STYLE, FALSE) == FALSE)
	{
		return nullptr;
	}

	const int WINDOW_WIDTH{ window_rect.right - window_rect.left };
	const int WINDOW_HEIGHT{ window_rect.bottom - window_rect.top };

	const int desktop_width{ GetSystemMetrics(SM_CXSCREEN) };
	const int desktop_height{ GetSystemMetrics(SM_CYSCREEN) };

	const int window_x{ std::max((desktop_width - WINDOW_WIDTH) / 2, 0) };
	const int window_y{ std::max((desktop_height - WINDOW_HEIGHT) / 2, 0) };

	HWND hWnd{ CreateWindow(
		WINDOW_CLASS,
		TITLE,
		WINDOW_STYLE,
		window_x,
		window_y,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	) };

	return hWnd;
}