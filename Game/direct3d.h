/*============================================================================
Contents   :  [direct3d.h]
              
Author     : Chin Qing You
LastUpdate : 2026/06/01
-----------------------------------------------------------------------------

============================================================================*/
#ifndef DIRECT3D_H
#define DIRECT3D_H

// safe release macro
#define SAFE_RELEASE(o) if (o) { (o)->Release(); o = NULL; } 
#include <Windows.h>
#include <d3d11.h>

bool Direct3DInitialize(HWND window_handle);
void Direct3DFinalize();
void Direct3D_Begin();
void Direct3D_Flip();

// Getters for Direct3D device and device context
ID3D11Device* Direct3D_GetDevice();
ID3D11DeviceContext* Direct3D_GetDeviceContext();

#endif
