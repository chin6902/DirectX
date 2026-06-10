/*============================================================================
Contents   :  [texture.h]
              
Author     : Chin Qing You
LastUpdate : 2026/06/10
-----------------------------------------------------------------------------
TextureManager
============================================================================*/
#ifndef TEXTURE_H 
#define TEXTURE_H

#include <d3d11.h>
#include <DirectXMath.h>

void Texture_Initialize(); 
void Texture_Finalize();

int Texture_Load(const wchar_t* pFileName, bool bMipMap = true);
constexpr int TEXTURE_INVALID_ID = -1;

void Texture_Release(int texture_id);
void Texture_ForceRelease(int texture_id);
void Texture_Release(const int* pTextureIDs, int count);
void Texture_AllRelease();

void Texture_SetTexture(int texture_id);

DirectX::XMUINT2 Texture_GetSize(int texture_id);
unsigned int Texture_GetWidth(int texture_id);
unsigned int Texture_GetHeight(int texture_id);

#endif