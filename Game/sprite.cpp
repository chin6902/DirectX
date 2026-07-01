/*============================================================================
Contents   :  [sprite.cpp]

Author     : Chin Qing You
LastUpdate : 2026/06/22
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
static ID3D11SamplerState* g_pSamplerState_Point{ nullptr };
static ID3D11SamplerState* g_pSamplerState_Linear{ nullptr };
static ID3D11BlendState* g_pBlendState{ nullptr };
static ID3D11DepthStencilState* g_pDepthStencilState{ nullptr };
static ID3D11Buffer* g_pVSConstantBuffer1{ nullptr };
static ID3D11Buffer* g_pPSConstantBuffer0{ nullptr };
static ID3D11RasterizerState* g_pRasterizerState{ nullptr };

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT2 texcoord;
};

static constexpr int NUM_VERTEX{ 4 };

// ============================================================================
// Initialize / Finalize
// ============================================================================
bool Sprite_Initialize()
{
    // --- Vertex buffer (unit quad, centred on origin) ---
    Vertex v[NUM_VERTEX]{};
    v[0].position = { -0.5f, -0.5f, 0.0f }; v[0].texcoord = { 0.0f, 0.0f };
    v[1].position = { 0.5f, -0.5f, 0.0f }; v[1].texcoord = { 1.0f, 0.0f };
    v[2].position = { -0.5f,  0.5f, 0.0f }; v[2].texcoord = { 0.0f, 1.0f };
    v[3].position = { 0.5f,  0.5f, 0.0f }; v[3].texcoord = { 1.0f, 1.0f };

    D3D11_BUFFER_DESC bd{
        .ByteWidth = sizeof(Vertex) * NUM_VERTEX,
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = 0
    };
    D3D11_SUBRESOURCE_DATA initData{ .pSysMem = v };

    HRESULT hr = Direct3D_GetDevice()->CreateBuffer(&bd, &initData, &g_pVertexBuffer);
    if (FAILED(hr))
    {
        hal::dout << "Sprite_Initialize(): 頂点バッファの作成に失敗しました" << std::endl;
        return false;
    }

    // --- Constant buffers ---
    D3D11_BUFFER_DESC cb{};
    cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    cb.ByteWidth = sizeof(XMFLOAT4X4);          // VS: world-view-proj matrix
    hr = Direct3D_GetDevice()->CreateBuffer(&cb, nullptr, &g_pVSConstantBuffer1);
    if (FAILED(hr)) { MessageBox(NULL, "UV行列の定数バッファ作成失敗", "エラー", MB_OK); return false; }

    cb.ByteWidth = sizeof(XMFLOAT4);            // PS: RGBA colour
    hr = Direct3D_GetDevice()->CreateBuffer(&cb, nullptr, &g_pPSConstantBuffer0);
    if (FAILED(hr)) { MessageBox(NULL, "カラーの定数バッファ作成失敗", "エラー", MB_OK); return false; }

    // --- Sampler states ---
    D3D11_SAMPLER_DESC sd{};
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0;
    sd.MaxLOD = D3D11_FLOAT32_MAX;

    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    Direct3D_GetDevice()->CreateSamplerState(&sd, &g_pSamplerState_Point);

    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    Direct3D_GetDevice()->CreateSamplerState(&sd, &g_pSamplerState_Linear);

    // --- Blend state (standard alpha blend) ---
    D3D11_BLEND_DESC blend{};
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    Direct3D_GetDevice()->CreateBlendState(&blend, &g_pBlendState);

    // --- Depth stencil (disabled for 2D sprites) ---
    D3D11_DEPTH_STENCIL_DESC dsd{};
    dsd.DepthEnable = FALSE;
    Direct3D_GetDevice()->CreateDepthStencilState(&dsd, &g_pDepthStencilState);

    // --- Rasterizer ---
    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    Direct3D_GetDevice()->CreateRasterizerState(&rd, &g_pRasterizerState);
    Direct3D_GetDeviceContext()->RSSetState(g_pRasterizerState);

    return true;
}

void Sprite_Finalize()
{
    Texture_AllRelease();
    SAFE_RELEASE(g_pRasterizerState);
    SAFE_RELEASE(g_pDepthStencilState);
    SAFE_RELEASE(g_pBlendState);
    SAFE_RELEASE(g_pSamplerState_Linear);
    SAFE_RELEASE(g_pSamplerState_Point);
    SAFE_RELEASE(g_pPSConstantBuffer0);
    SAFE_RELEASE(g_pVSConstantBuffer1);
    SAFE_RELEASE(g_pVertexBuffer);
}

void Sprite_SetFilter(SpriteFilter filter)
{
    ID3D11SamplerState* sampler = (filter == kSpriteFilter_Linear) ? g_pSamplerState_Linear : g_pSamplerState_Point;
    Direct3D_GetDeviceContext()->PSSetSamplers(0, 1, &sampler);
}

// ============================================================================
// Internal draw — every public overload funnels into this one function
//
// Parameters
//   texture_id          : loaded texture handle
//   x, y               : screen position (top-left of the sprite)
//   width, height       : on-screen draw size in pixels
//   texture_x/y         : top-left of the source region inside the texture
//   texture_width/height: size of the source region inside the texture
//   params              : optional tint / alpha / scale / rotation
// ============================================================================
static void Sprite_DrawInternal(
    int   texture_id,
    float x, float y,
    float width, float height,
    float texture_x, float texture_y,
    int   texture_width, int texture_height,
    const SpriteDrawParams& params)
{
    Shader_Begin();

    // --- World transform (scale → rotate → translate → project) ---
    //
    // flip_x/y negate the scale axis to mirror the quad.
    // The pivot (cx, cy) deliberately uses the *positive* scale so the
    // sprite's top-left corner stays at (x, y) whether flipped or not.
    float scaleX = params.flip_x ? -(params.scale.x) : params.scale.x;
    float scaleY = params.flip_y ? -(params.scale.y) : params.scale.y;

    XMMATRIX mtxScale = XMMatrixScaling(width * scaleX, height * scaleY, 1.0f);
    XMMATRIX mtxRotation = XMMatrixRotationZ(params.angle);

    float cx = x + width * params.scale.x * 0.5f;   
    float cy = y + height * params.scale.y * 0.5f;
    XMMATRIX mtxTranslation = XMMatrixTranslation(cx, cy, 0.0f);
    XMMATRIX mtxProjection = XMMatrixOrthographicOffCenterLH(
        0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);

    XMMATRIX mtx = mtxScale * mtxRotation * mtxTranslation * mtxProjection;
    Shader_SetMatrix(mtx);

    // --- UV transform ---
    float tx = texture_x / Texture_GetWidth(texture_id);
    float ty = texture_y / Texture_GetHeight(texture_id);
    float tw = texture_width / (float)Texture_GetWidth(texture_id);
    float th = texture_height / (float)Texture_GetHeight(texture_id);

    XMMATRIX mtxUVScale = XMMatrixScaling(tw, th, 1.0f);
    XMMATRIX mtxUVTranslation = XMMatrixTranslation(tx, ty, 0.0f);
    XMFLOAT4X4 mtxUV;
    XMStoreFloat4x4(&mtxUV, XMMatrixTranspose(mtxUVScale * mtxUVTranslation));

    Direct3D_GetDeviceContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &mtxUV, 0, 0);
    Direct3D_GetDeviceContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);

    // --- Colour + alpha packed into XMFLOAT4 for the pixel shader ---
    XMFLOAT4 rgba{ params.color.x, params.color.y, params.color.z, params.alpha };
    Direct3D_GetDeviceContext()->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &rgba, 0, 0);
    Direct3D_GetDeviceContext()->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

    // --- Geometry ---
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    Texture_SetTexture(texture_id);

    Direct3D_GetDeviceContext()->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);
    Direct3D_GetDeviceContext()->OMSetDepthStencilState(g_pDepthStencilState, 0);

    Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}

static const SpriteDrawParams DEFAULT_PARAMS{};  // white, alpha=1, scale=1, angle=0

// (1) full texture, default params
void Sprite_Draw(int texture_id, float x, float y)
{
    Sprite_Draw(texture_id, x, y, DEFAULT_PARAMS);
}

// (1p) full texture, custom params
void Sprite_Draw(int texture_id, float x, float y,
    const SpriteDrawParams& params)
{
    float w = static_cast<float>(Texture_GetWidth(texture_id));
    float h = static_cast<float>(Texture_GetHeight(texture_id));
    Sprite_DrawInternal(texture_id, x, y, w, h, 0.0f, 0.0f,
        Texture_GetWidth(texture_id),
        Texture_GetHeight(texture_id),
        params);
}

// (2) sized, default params
void Sprite_Draw(int texture_id, float x, float y, float width, float height)
{
    Sprite_Draw(texture_id, x, y, width, height, DEFAULT_PARAMS);
}

// (2p) sized, custom params
void Sprite_Draw(int texture_id, float x, float y,
    float width, float height,
    const SpriteDrawParams& params)
{
    Sprite_DrawInternal(texture_id, x, y, width, height, 0.0f, 0.0f,
        Texture_GetWidth(texture_id),
        Texture_GetHeight(texture_id),
        params);
}

// (3) sub-region, default params
void Sprite_Draw(int texture_id, float x, float y,
    float width, float height,
    float texture_x, float texture_y,
    int texture_width, int texture_height)
{
    Sprite_Draw(texture_id, x, y, width, height,
        texture_x, texture_y, texture_width, texture_height,
        DEFAULT_PARAMS);
}

// (3p) sub-region, custom params  ← the one internal function actually executes
void Sprite_Draw(int texture_id, float x, float y,
    float width, float height,
    float texture_x, float texture_y,
    int texture_width, int texture_height,
    const SpriteDrawParams& params)
{
    Sprite_DrawInternal(texture_id, x, y, width, height,
        texture_x, texture_y, texture_width, texture_height,
        params);
}