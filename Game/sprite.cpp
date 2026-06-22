/*============================================================================
Contents   :  [sprite.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/15
-----------------------------------------------------------------------------
//instancing & render state management (update needed)
//make switch statements for settings 
============================================================================*/
#include "debug_ostream.h"
#include "config.h"
#include "sprite.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"

using namespace DirectX;

static ID3D11Buffer* g_pVertexBuffer{ nullptr };
static ID3D11SamplerState* g_pSamplerState_Point{ nullptr };
static ID3D11SamplerState* g_pSamplerState_Linear{ nullptr };

static ID3D11BlendState* g_pBlendState{ nullptr };
static ID3D11DepthStencilState* g_pDepthStencilState{ nullptr };
static ID3D11Buffer* g_pVSConstantBuffer1{ nullptr }; 
static ID3D11Buffer* g_pPSConstantBuffer0{ nullptr }; 
static ID3D11RasterizerState* g_pRasterizerState{ nullptr };

// 頂点構造体
struct Vertex
{
    XMFLOAT3 position; //(x, y, z)
	XMFLOAT2 texcoord; //(u, v)
};

static constexpr int NUM_VERTEX{ 4 };

bool Sprite_Initialize()
{
    D3D11_BUFFER_DESC bd{
        .ByteWidth = sizeof(Vertex) * NUM_VERTEX,
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = 0
    };

    //頂点バッファ
    Vertex v[NUM_VERTEX]{};

    v[0].position = { -0.5f, -0.5f, 0.0f }; // 左上
    v[1].position = { 0.5f, -0.5f, 0.0f }; // 右上
    v[2].position = { -0.5f, 0.5f, 0.0f }; // 左下
    v[3].position = { 0.5f, 0.5f, 0.0f }; // 右下

    v[0].texcoord = { 0.0f, 0.0f };
    v[1].texcoord = { 1.0f, 0.0f };
    v[2].texcoord = { 0.0f, 1.0f };
    v[3].texcoord = { 1.0f, 1.0f };

    D3D11_SUBRESOURCE_DATA initData{}; 
    initData.pSysMem = v;

    HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &initData, &g_pVertexBuffer);

    if (FAILED(hr))
    {
        hal::dout << "Sprite_Initialize() : 頂点バッファの作成に失敗しました" << std::endl;
        return false;
    }

    // 定数バッファ UV
    D3D11_BUFFER_DESC cb_desc{};
    cb_desc.ByteWidth = sizeof(XMFLOAT4X4);// バッファのサイズ (16の倍数であること)
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    // 頂点シェーダー用定数バッファの作成
    hr = Direct3D_GetDevice()->CreateBuffer(&cb_desc, nullptr, &g_pVSConstantBuffer1); 
    
    if (FAILED(hr)) 
    { 
        MessageBox(NULL, "UV用行列の定数バッファの作成に失敗しました", "エラー", MB_OK); 
        return false; 
    }

    // 定数バッファ color
    cb_desc.ByteWidth = sizeof(XMFLOAT4);
    hr = Direct3D_GetDevice()->CreateBuffer(&cb_desc, nullptr, &g_pPSConstantBuffer0);
    if (FAILED(hr))
    {
        MessageBox(NULL, "color用の定数バッファの作成に失敗しました", "エラー", MB_OK);
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

    Direct3D_GetDevice()->CreateSamplerState(&sd, &g_pSamplerState_Point);

    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    Direct3D_GetDevice()->CreateSamplerState(&sd, &g_pSamplerState_Linear);

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

    // ラスタライザーステートの作成 
    D3D11_RASTERIZER_DESC rasterizer_desc{};
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = D3D11_CULL_NONE;
    //rasterizer_desc.FrontCounterClockwise = FALSE;

    Direct3D_GetDevice()->CreateRasterizerState(&rasterizer_desc, &g_pRasterizerState);
    Direct3D_GetDeviceContext()->RSSetState(g_pRasterizerState);
    
    return true;
}

void Sprite_Finalize()
{
    Texture_AllRelease();
    SAFE_RELEASE(g_pRasterizerState);
    SAFE_RELEASE(g_pDepthStencilState);
    SAFE_RELEASE(g_pBlendState);
    SAFE_RELEASE(g_pSamplerState_Point);
    SAFE_RELEASE(g_pSamplerState_Linear);
    SAFE_RELEASE(g_pPSConstantBuffer0);
	SAFE_RELEASE(g_pVSConstantBuffer1);
    SAFE_RELEASE(g_pVertexBuffer);
}

void Sprite_SetFilter(SpriteFilter filter)
{
    switch (filter)
    {
    case kSpriteFilter_Point:
        //第一引数はスロット番号(シェーダー側のregister(s0))
        Direct3D_GetDeviceContext()->PSSetSamplers(0, 1, &g_pSamplerState_Point);
        break;
    case kSpriteFilter_Linear:
        Direct3D_GetDeviceContext()->PSSetSamplers(0, 1, &g_pSamplerState_Linear);
        break;
    default:
        Direct3D_GetDeviceContext()->PSSetSamplers(0, 1, &g_pSamplerState_Point);
        break;
    }
}

void Sprite_Draw(int texture_ID, float x, float y, const DirectX::XMFLOAT4& color)
{
	Sprite_Draw(texture_ID, x, y, (float)Texture_GetWidth(texture_ID), (float)Texture_GetHeight(texture_ID), color);
}

void Sprite_Draw(int texture_ID, float x, float y, float width, float height, const DirectX::XMFLOAT4& color)
{
    //シェーダーを設定する
    Shader_Begin();

    //変換行列を設定する
    XMMATRIX mtxScaling = XMMatrixScaling(width, height, 1.0f);
    XMMATRIX mtxTranslation = XMMatrixTranslation(x + width * 0.5f, y + height * 0.5f, 0.0f);
    XMMATRIX mtxProjection = XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);

    XMMATRIX mtx = mtxScaling * mtxTranslation * mtxProjection; //行列を合成

    Shader_SetMatrix(mtx);

    Direct3D_GetDeviceContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &mtx, 0, 0);
    Direct3D_GetDeviceContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);

    Direct3D_GetDeviceContext()->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
    Direct3D_GetDeviceContext()->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

    //頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    //プリミティブトポロジー設定
    Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    //ピクセルシェーダーにテクスチャを設定する
    Texture_SetTexture(texture_ID);

    //ブレンドステートの設定
    Direct3D_GetDeviceContext()->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);

    // デプスステンシルステートをパイプライン（OMステージ）に設定 (calling every frame is not good for performance)
    Direct3D_GetDeviceContext()->OMSetDepthStencilState(g_pDepthStencilState, 0);

    Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}

void Sprite_Draw(int texture_ID, float x, float y, float texture_x, float texture_y, int texture_width, int texture_height, const DirectX::XMFLOAT4& color)
{
    Sprite_Draw(texture_ID, x, y, static_cast<float>(texture_width), static_cast<float>(texture_height), texture_x, texture_y, texture_width, texture_height, color);
}

void Sprite_Draw(int texture_ID, float x, float y, float width, float height, float texture_x, float texture_y, int texture_width, int texture_height, const DirectX::XMFLOAT4& color)
{
    //シェーダーを設定する
    Shader_Begin();

	//変換行列を設定する
	XMMATRIX mtxScaling = XMMatrixScaling(width, height, 1.0f);
	XMMATRIX mtxTranslation = XMMatrixTranslation(x + width * 0.5f, y + height * 0.5f, 0.0f);
	XMMATRIX mtxProjection = XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);

	XMMATRIX mtx = mtxScaling * mtxTranslation * mtxProjection; //行列を合成

    //座標変換用行列を頂点シェーダーに設定する
    Shader_SetMatrix(mtx);

    //UV用行列を定数バッファーへ送る
    float tx = texture_x / Texture_GetWidth(texture_ID);
    float ty = texture_y / Texture_GetHeight(texture_ID);
    float tw = texture_width / (float)Texture_GetWidth(texture_ID);
    float th = texture_height / (float)Texture_GetHeight(texture_ID);
    mtxScaling = XMMatrixScaling(tw, th, 1.0f);
    mtxTranslation = XMMatrixTranslation(tx, ty, 0.0f);
    XMFLOAT4X4 mtxUV;
    XMStoreFloat4x4(&mtxUV, XMMatrixTranspose(mtxScaling * mtxTranslation));

    Direct3D_GetDeviceContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &mtxUV, 0, 0);
    Direct3D_GetDeviceContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);

    Direct3D_GetDeviceContext()->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
    Direct3D_GetDeviceContext()->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

    //頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    //プリミティブトポロジー設定
    Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    //ピクセルシェーダーにテクスチャを設定する
    Texture_SetTexture(texture_ID);

    //ブレンドステートの設定
    Direct3D_GetDeviceContext()->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);

    // デプスステンシルステートをパイプライン（OMステージ）に設定 (calling every frame is not good for performance)
    Direct3D_GetDeviceContext()->OMSetDepthStencilState(g_pDepthStencilState, 0);

    Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}

void Sprite_Draw(int texture_ID, float x, float y, float width, float height, float texture_x, float texture_y, int texture_width, int texture_height, float angle, const DirectX::XMFLOAT2& scale, const DirectX::XMFLOAT4& color)
{
    //シェーダーを設定する
    Shader_Begin();

    //変換行列を設定する
    XMMATRIX mtxScaling = XMMatrixScaling(width * scale.x, height * scale.y, 1.0f);
    XMMATRIX mtxRotation = XMMatrixRotationZ(angle);
    XMMATRIX mtxTranslation = XMMatrixTranslation(x + width * 0.5f, y + height * 0.5f, 0.0f);
    XMMATRIX mtxProjection = XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);

    XMMATRIX mtx = mtxScaling * mtxRotation * mtxTranslation * mtxProjection; //行列を合成

    //座標変換用行列を頂点シェーダーに設定する
    Shader_SetMatrix(mtx);

    //UV用行列を定数バッファーへ送る
    float tx = texture_x / Texture_GetWidth(texture_ID);
    float ty = texture_y / Texture_GetHeight(texture_ID);
    float tw = texture_width / (float)Texture_GetWidth(texture_ID);
    float th = texture_height / (float)Texture_GetHeight(texture_ID);
    mtxScaling = XMMatrixScaling(tw, th, 1.0f);
    mtxTranslation = XMMatrixTranslation(tx, ty, 0.0f);
    XMFLOAT4X4 mtxUV;
    XMStoreFloat4x4(&mtxUV, XMMatrixTranspose(mtxScaling * mtxTranslation));

    //UV
    Direct3D_GetDeviceContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &mtxUV, 0, 0);
    Direct3D_GetDeviceContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);

    //color
    Direct3D_GetDeviceContext()->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
    Direct3D_GetDeviceContext()->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

    //頂点バッファを描画パイプラインに設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

    //プリミティブトポロジー設定
    Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    //ピクセルシェーダーにテクスチャを設定する
    Texture_SetTexture(texture_ID);

    //ブレンドステートの設定
    Direct3D_GetDeviceContext()->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);

    // デプスステンシルステートをパイプライン（OMステージ）に設定 (calling every frame is not good for performance)
    Direct3D_GetDeviceContext()->OMSetDepthStencilState(g_pDepthStencilState, 0);

    Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}
