/*==============================================================================

   マウス入力ラッパー [input_mouse.cpp]
                                                         Author : Youhei Sato
--------------------------------------------------------------------------------

==============================================================================*/
#include "input_mouse.h"
#include <cstring>

// 前フレームのマウスボタン状態
static bool gPrevMouseButton[MOUSE_BUTTON_MAX] = {};

// 現在フレームのマウス状態
static Mouse_State gMouseState = {};


// ========== 初期化 / 更新 / 終了 ==========

void InputMouse_Initialize(HWND hWnd)
{
    Mouse_Initialize(hWnd);
    std::memset(&gMouseState, 0, sizeof(gMouseState));
    std::memset(gPrevMouseButton, 0, sizeof(gPrevMouseButton));
}

void InputMouse_Update(void)
{
    // 前フレームのボタン状態を保存
    gPrevMouseButton[MOUSE_BUTTON_LEFT]   = gMouseState.leftButton;
    gPrevMouseButton[MOUSE_BUTTON_RIGHT]  = gMouseState.rightButton;
    gPrevMouseButton[MOUSE_BUTTON_MIDDLE] = gMouseState.middleButton;
	gPrevMouseButton[MOUSE_BUTTON_X1] = gMouseState.xButton1;
	gPrevMouseButton[MOUSE_BUTTON_X2] = gMouseState.xButton2; 

    // 現在のマウス状態を取得
    Mouse_GetState(&gMouseState);

    // スクロールホイール値のリセットを自動化
    Mouse_ResetScrollWheelValue();
}

void InputMouse_Finalize(void)
{
    Mouse_Finalize();
}


// ========== 判定処理 ==========

bool InputMouse_IsPress(MouseButton button)
{
    switch (button)
    {
    case MOUSE_BUTTON_LEFT:   return gMouseState.leftButton;
    case MOUSE_BUTTON_RIGHT:  return gMouseState.rightButton;
    case MOUSE_BUTTON_MIDDLE: return gMouseState.middleButton;
    case MOUSE_BUTTON_X1:     return gMouseState.xButton1;
    case MOUSE_BUTTON_X2:     return gMouseState.xButton2;
    default:                  return false;
    }
}

bool InputMouse_IsTrigger(MouseButton button)
{
    if (button < 0 || button >= MOUSE_BUTTON_MAX) return false;
    bool prev = gPrevMouseButton[button];
    bool curr = InputMouse_IsPress(button);
    return (!prev && curr);
}

bool InputMouse_IsRelease(MouseButton button)
{
    if (button < 0 || button >= MOUSE_BUTTON_MAX) return false;
    bool prev = gPrevMouseButton[button];
    bool curr = InputMouse_IsPress(button);
    return (prev && !curr);
}

int InputMouse_GetX(void)
{
    return gMouseState.x;
}

int InputMouse_GetY(void)
{
    return gMouseState.y;
}

int InputMouse_GetScrollWheel(void)
{
    return gMouseState.scrollWheelValue;
}

void InputMouse_SetMode(Mouse_PositionMode mode)
{
    Mouse_SetMode(mode);
}

void InputMouse_SetVisible(bool visible)
{
    Mouse_SetVisible(visible);
}