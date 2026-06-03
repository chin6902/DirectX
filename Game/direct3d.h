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

bool Direct3DInitialize(HWND window_handle);
void Direct3DFinalize();
void Direct3D_Begin();
void Direct3D_Flip();

#endif
