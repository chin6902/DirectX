/*============================================================================
Contents   :  [sprite.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/15
-----------------------------------------------------------------------------

============================================================================*/
#include "debug_ostream.h"
#include "config.h"
#include "sprite.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"

using namespace DirectX;

static ID3D11Buffer* g_pVertexBuffer{ nullptr };
static ID3D11SamplerState* g_pSamplerState{ nullptr };
static ID3D11BlendState* g_pBlendState{ nullptr };
static ID3D11DepthStencilState* g_pDepthStencilState{ nullptr };

// 頂点構造体
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
    XMFLOAT2 texcoord;
};

static constexpr int NUM_VERTEX{ 4 };

bool Sprite_Initialize()
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
        hal::dout << "Sprite_Initialize() : 頂点バッファの作成に失敗しました" << std::endl;
        return false;
    }

    D3D11_SAMPLER_DESC sd{};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT; // フィルタリング設定 
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;  // 範囲外の扱い (横方向)
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;  // 範囲外の扱い (縦方向)
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;  // 範囲外の扱い (奥行方向)
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

    // デプスステンシルステートの設定
    D3D11_DEPTH_STENCIL_DESC dsd{};
    dsd.DepthEnable = FALSE; // デプステストを無効化
    /*
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // デプス書き込みを無効化(書き込まない)
    dsd.DepthFunc = D3D11_COMPARISON_NEVER; // 比較処理を行わない
    dsd.StencilEnable = FALSE; // ステンシルテスト無効
    */

    Direct3D_GetDevice()->CreateDepthStencilState(&dsd, &g_pDepthStencilState);
    
    return true;
}

void Sprite_Finalize()
{
    Texture_AllRelease();
    SAFE_RELEASE(g_pDepthStencilState);
    SAFE_RELEASE(g_pBlendState);
    SAFE_RELEASE(g_pSamplerState);
    SAFE_RELEASE(g_pVertexBuffer);
}

void Sprite_Draw(int texture_ID, float x, float y, const DirectX::XMFLOAT4& color)
{
	Sprite_Draw(texture_ID, x, y, (float)Texture_GetWidth(texture_ID), (float)Texture_GetHeight(texture_ID), color);
}

void Sprite_Draw(int texture_ID, float x, float y, float width, float height, const DirectX::XMFLOAT4& color)
{
    //シェーダーを設定する
    Shader_Begin();

    //座標変換用行列を頂点シェーダーに設定する
    Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

    //頂点バッファをロックする
    D3D11_MAPPED_SUBRESOURCE msr;
    HRESULT hr = Direct3D_GetDeviceContext()->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    if (SUCCEEDED(hr))
    {
        //頂点バッファへの仮想アドレスを取得
        Vertex* v = (Vertex*)msr.pData;

        // x, y 座標とテクスチャサイズに基づき各頂点座標を設定
        v[0].position = { x, y, 0.0f }; // 左上
        v[1].position = { x + width, y, 0.0f }; // 右上
        v[2].position = { x, y + height, 0.0f }; // 左下
        v[3].position = { x + width, y + height, 0.0f }; // 右下

        // 頂点カラー 
        v[0].color = color;
        v[1].color = color;
        v[2].color = color;
        v[3].color = color;

        v[0].texcoord = { 0.0f, 0.0f };
        v[1].texcoord = { 1.0f, 0.0f };
        v[2].texcoord = { 0.0f, 1.0f };
        v[3].texcoord = { 1.0f, 1.0f };

        //頂点バッファのロックを解除
        Direct3D_GetDeviceContext()->Unmap(g_pVertexBuffer, 0);
    }

    //頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    //プリミティブトポロジー設定
    Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    //第一引数はスロット番号(シェーダー側のregister(s0))
    Direct3D_GetDeviceContext()->PSSetSamplers(0, 1, &g_pSamplerState);

    //ピクセルシェーダーにテクスチャを設定する
    Texture_SetTexture(texture_ID);

    //ブレンドステートの設定
    Direct3D_GetDeviceContext()->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);

    // デプスステンシルステートをパイプライン（OMステージ）に設定 (calling every frame is not good for performance)
    Direct3D_GetDeviceContext()->OMSetDepthStencilState(g_pDepthStencilState, 0);

    Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}

void Sprite_Draw(int texture_ID, float x, float y, int texture_x, int texture_y, int texture_width, int texture_height, const DirectX::XMFLOAT4& color)
{
    Sprite_Draw(texture_ID, x, y, texture_width, texture_height, texture_x, texture_y, texture_width, texture_height, color);
}

void Sprite_Draw(int texture_ID, float x, float y, float width, float height, int texture_x, int texture_y, int texture_width, int texture_height, const DirectX::XMFLOAT4& color)
{
    //シェーダーを設定する
    Shader_Begin();

    //座標変換用行列を頂点シェーダーに設定する
    Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

    //頂点バッファをロックする
    D3D11_MAPPED_SUBRESOURCE msr;
    HRESULT hr = Direct3D_GetDeviceContext()->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

    if (SUCCEEDED(hr))
    {
        //頂点バッファへの仮想アドレスを取得
        Vertex* v = (Vertex*)msr.pData;

        // x, y 座標とテクスチャサイズに基づき各頂点座標を設定
        v[0].position = { x, y, 0.0f }; // 左上
        v[1].position = { x + width, y, 0.0f }; // 右上
        v[2].position = { x, y + height, 0.0f }; // 左下
        v[3].position = { x + width, y + height, 0.0f }; // 右下
        
        // 頂点カラー 
		v[0].color = color; 
		v[1].color = color; 
		v[2].color = color; 
		v[3].color = color; 

        // テクスチャの切り取り範囲に基づいて UV 座標を計算して設定
        float u0 = static_cast<float>(texture_x) / Texture_GetWidth(texture_ID);
        float v0 = static_cast<float>(texture_y) / Texture_GetHeight(texture_ID);
        float u1 = u0 + static_cast<float>(texture_width) / Texture_GetWidth(texture_ID);
        float v1 = v0 + static_cast<float>(texture_height) / Texture_GetHeight(texture_ID);
        v[0].texcoord = { u0, v0 };
        v[1].texcoord = { u1, v0 };
        v[2].texcoord = { u0, v1 };
        v[3].texcoord = { u1, v1 };

        //頂点バッファのロックを解除
        Direct3D_GetDeviceContext()->Unmap(g_pVertexBuffer, 0);
    }

    //頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    //プリミティブトポロジー設定
    Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    //第一引数はスロット番号(シェーダー側のregister(s0))
    Direct3D_GetDeviceContext()->PSSetSamplers(0, 1, &g_pSamplerState);

    //ピクセルシェーダーにテクスチャを設定する
    Texture_SetTexture(texture_ID);

    //ブレンドステートの設定
    Direct3D_GetDeviceContext()->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);

    // デプスステンシルステートをパイプライン（OMステージ）に設定 (calling every frame is not good for performance)
    Direct3D_GetDeviceContext()->OMSetDepthStencilState(g_pDepthStencilState, 0);

    Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}
