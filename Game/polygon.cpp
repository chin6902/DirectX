/*============================================================================
Contents   :  [polygon.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/08
-----------------------------------------------------------------------------
practice file
============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>

#include "debug_ostream.h"
#include "config.h"
#include "polygon.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"

using namespace DirectX;

static ID3D11Buffer* g_pVertexBuffer{ nullptr };
static ID3D11SamplerState* g_pSamplerState{ nullptr };
static ID3D11BlendState* g_pBlendState{ nullptr };
static int g_TextureID{ TEXTURE_INVALID_ID };

// 頂点構造体
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
    XMFLOAT2 texcoord;
};

static constexpr int NUM_VERTEX { 4 };

bool Polygon_Initialize()
{
    D3D11_BUFFER_DESC bd{
        .ByteWidth = sizeof(Vertex) * NUM_VERTEX,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
    }; 


    HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, NULL, &g_pVertexBuffer);

	if (FAILED(hr))
	{
		hal::dout << "Polygon_Initialize() : 頂点バッファの作成に失敗しました" << std::endl;
		return false;
	}

	// テクスチャの読み込み
	g_TextureID = Texture_Load(L"ochaduke.png", false);

    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // フィルタリング設定 (MIPMAPリニア)
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;  // 範囲外の扱い (横方向：クランプ)
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;  // 範囲外の扱い (縦方向：クランプ)
    sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;  // 範囲外の扱い (奥行方向：クランプ)
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;

    Direct3D_GetDevice()->CreateSamplerState(&sd, &g_pSamplerState);

	// ブレンドステートの設定(半透明の設定)
    D3D11_BLEND_DESC blend_desc{};
    blend_desc.RenderTarget[0].BlendEnable = TRUE;

	blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; // 描画する色の係数
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; // すでにある色の係数
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;

    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	//rgb = src_rgb * src_alpha + dest_rgb * (1 - src_alpha)

    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    Direct3D_GetDevice()->CreateBlendState(&blend_desc, &g_pBlendState);

    return true;
}

void Polygon_Finalize()
{
    Texture_AllRelease();
	SAFE_RELEASE(g_pBlendState);
    SAFE_RELEASE(g_pSamplerState);
	SAFE_RELEASE(g_pVertexBuffer);

}

void Polygon_Draw()
{
	//シェーダーを設定する
	Shader_Begin();

	//座標変換用行列を頂点シェーダーに設定する
    Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

    //頂点バッファをロックする
    D3D11_MAPPED_SUBRESOURCE msr;
    Direct3D_GetDeviceContext()->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    //頂点バッファへの仮想アドレスを取得
    Vertex* v = (Vertex*)msr.pData; 

    //画面の左上から右下に向かう線分を描画する
    v[0].position = { 100.0f, 100.0f, 0.0f };
    v[0].color = { 1.0f, 0.0f, 0.0f, 1.0f }; // 赤
    v[0].texcoord = { 0.0f , 0.0f };

    v[1].position = { 500.0f, 100.0f, 0.0f };
    v[1].color = { 0.0f, 1.0f, 0.0f, 1.0f }; // 緑  
    v[1].texcoord = { 1.0f , 0.0f };

    v[2].position = { 100.0f, 500.0f, 0.0f };
    v[2].color = { 0.0f, 0.0f, 1.0f, 1.0f }; // 青
    v[2].texcoord = { 0.0f , 1.0f };

    v[3].position = { 500.0f, 500.0f, 0.0f };
    v[3].color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色
    v[3].texcoord = { 1.0f , 1.0f };
    //v[4].position = { 600.0f, 100.0f, 0.0f };
    //v[5].position = { 600.0f, 500.0f, 0.0f };

    //頂点バッファのロックを解除
    Direct3D_GetDeviceContext()->Unmap(g_pVertexBuffer, 0);

    //頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	//プリミティブトポロジー設定
    Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    //第一引数はスロット番号(シェーダー側のregister(s0))
    Direct3D_GetDeviceContext()->PSSetSamplers(0, 1, &g_pSamplerState);

	//ピクセルシェーダーにテクスチャを設定する
	Texture_SetTexture(g_TextureID);

	//ブレンドステートの設定
    Direct3D_GetDeviceContext()->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);

    Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}
