/*============================================================================
Contents   :  [texture.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/10
-----------------------------------------------------------------------------
TextureManager
============================================================================*/
#include <string>
#include <DirectXMath.h>

#include "texture.h"
#include "direct3d.h"
#include "WICTextureLoader11.h"

using namespace DirectX;

static constexpr int TEXTURE_MAX = 1024;

struct Texture
{
    std::wstring filename;
	unsigned int width = 0;
	unsigned int height = 0;
	int ref_count = 0;

	ID3D11Resource* pTexture{ nullptr };
	ID3D11ShaderResourceView* pTextureView{ nullptr };
};

static Texture g_Textures[TEXTURE_MAX]{};

void Texture_Initialize()
{
}

void Texture_Finalize()
{
	Texture_AllRelease();
}

int Texture_Load(const wchar_t* pFileName, bool bMipMap)
{
	if (!pFileName || pFileName[0] == L'\0') 
	{ 
		return TEXTURE_INVALID_ID; 
	}

	for (int i = 0; i < TEXTURE_MAX; i++) {
		if (!g_Textures[i].pTexture) continue; 

		// 同じファイル名が見つかったら、そのIDを返す（新しく作らない）
		if (g_Textures[i].filename == pFileName)
		{ 
			g_Textures[i].ref_count++;
			return i; 
		} 
	}

	for (int i = 0; i < TEXTURE_MAX; i++)
	{ // 空いている場所を探す
		if (g_Textures[i].pTexture) continue;

		HRESULT hr; // ミップマップの有無で読み込み関数を分ける

		if (bMipMap)
		{
			hr = CreateWICTextureFromFile(Direct3D_GetDevice(), Direct3D_GetDeviceContext(), pFileName, &g_Textures[i].pTexture, &g_Textures[i].pTextureView);
		}
		else
		{
			hr = CreateWICTextureFromFile(Direct3D_GetDevice(), pFileName, &g_Textures[i].pTexture, &g_Textures[i].pTextureView);
		}

		if (FAILED(hr))
		{
			MessageBoxW(nullptr, L"テクスチャの読み込みに失敗しました", pFileName, MB_OK | MB_ICONERROR);
			break;
		}

		ID3D11Texture2D* pTexture = (ID3D11Texture2D*)g_Textures[i].pTexture;
		D3D11_TEXTURE2D_DESC t2desc;
		pTexture->GetDesc(&t2desc);
		g_Textures[i].width = t2desc.Width;
		g_Textures[i].height = t2desc.Height;

		g_Textures[i].filename = pFileName;
		g_Textures[i].ref_count = 1;

		return i;
	}

	return TEXTURE_INVALID_ID;
}

void Texture_SetTexture(int texture_id)
{
	if (texture_id < 0 || texture_id >= TEXTURE_MAX) 
	{ 
		return; 
	}

	Direct3D_GetDeviceContext()->PSSetShaderResources(0, 1, &g_Textures[texture_id].pTextureView);
}

void Texture_ForceRelease(int texture_id) 
{
	if (texture_id < 0 || texture_id >= TEXTURE_MAX)
	{
		return;
	}

	Texture& t = g_Textures[texture_id];

	SAFE_RELEASE(t.pTextureView);
	SAFE_RELEASE(t.pTexture);

	t.filename.clear();
	t.width = 0;
	t.height = 0;
	t.ref_count = 0;
}

void Texture_Release(int texture_id)
{
	if (texture_id < 0 || texture_id >= TEXTURE_MAX) 
	{ 
		return; 
	} 
	
	if (!g_Textures[texture_id].pTexture)
	{
		return;
	}

	g_Textures[texture_id].ref_count--;

	if (g_Textures[texture_id].ref_count > 0)
	{
		return;
	}

	Texture_ForceRelease(texture_id);
}

void Texture_Release(const int* pTextureIDs, int count)
{
	if (!pTextureIDs || count <= 0) 
	{ 
		return; 
	}
	
	for (int i = 0; i < count; i++) 
	{ 
		if (pTextureIDs[i] < 0 || pTextureIDs[i] >= TEXTURE_MAX) 
		{ 
			continue;
		}

		Texture_Release(pTextureIDs[i]); 
	}
}

void Texture_AllRelease()
{
	for (int i = 0; i < TEXTURE_MAX; i++)
	{
		Texture_ForceRelease(i); 
	}
}

DirectX::XMUINT2 Texture_GetSize(int texture_id)
{
	if (texture_id < 0 || texture_id >= TEXTURE_MAX)
	{ 
		return { 0, 0 }; 
	} 
	
	return { g_Textures[texture_id].width, g_Textures[texture_id].height };
}

unsigned int Texture_GetWidth(int texture_id)
{
	if (texture_id < 0 || texture_id >= TEXTURE_MAX) 
	{ 
		return 0; 
	} 
	
	return g_Textures[texture_id].width;
}

unsigned int Texture_GetHeight(int texture_id)
{
	if (texture_id < 0 || texture_id >= TEXTURE_MAX) 
	{ 
		return 0; 
	} 
	
	return g_Textures[texture_id].height;
}
