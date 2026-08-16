/*============================================================================
Contents   :  [collision_debug.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/07/06
-----------------------------------------------------------------------------
rewrite it for more efficiency(make shader for debug circle)
============================================================================*/
#include <DirectXMath.h>

#include "collision_debug.h"
#include "debug_ostream.h"
#include "config.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"

using namespace DirectX;

static ID3D11Buffer* g_pVertexBuffer{ nullptr };
static ID3D11SamplerState* g_pSamplerState_Point{ nullptr };
static ID3D11BlendState* g_pBlendState{ nullptr };
static ID3D11DepthStencilState* g_pDepthStencilState{ nullptr };
static ID3D11Buffer* g_pVSConstantBuffer1{ nullptr };
static ID3D11Buffer* g_pPSConstantBuffer0{ nullptr };

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT2 texcoord;
};

static constexpr int NUM_VERTEX{ 17 };

static int g_WhiteTextureID{ -1 };

void Collision_Debug_Initialize()
{
    // --- Vertex buffer (unit quad, centred on origin) ---
    Vertex v[NUM_VERTEX]{};

	constexpr float angle = XM_2PI / 16.0f; // 360 degrees divided by 16 segments
	for (int i = 0; i < 17; ++i)
	{
		const float a = angle * i;
		v[i].position.x = cosf(a) * 1.0f;
		v[i].position.y = sinf(a) * 1.0f;
        v[i].position.z = 0.0f;
        v[i].texcoord = { 0.0f, 0.0f };
    }

    D3D11_BUFFER_DESC bd{
        .ByteWidth = sizeof(Vertex) * NUM_VERTEX,
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = 0
    };
    D3D11_SUBRESOURCE_DATA initData{ .pSysMem = v };

    Direct3D_GetDevice()->CreateBuffer(&bd, &initData, &g_pVertexBuffer);

    // --- Constant buffers ---
    D3D11_BUFFER_DESC cb{};
    cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    cb.ByteWidth = sizeof(XMFLOAT4X4);          // VS: world-view-proj matrix
    Direct3D_GetDevice()->CreateBuffer(&cb, nullptr, &g_pVSConstantBuffer1);

    cb.ByteWidth = sizeof(XMFLOAT4);            // PS: RGBA colour
    Direct3D_GetDevice()->CreateBuffer(&cb, nullptr, &g_pPSConstantBuffer0);

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

    // --- Blend state (standard alpha blend) ---
    D3D11_BLEND_DESC blend{};
    blend.RenderTarget[0].BlendEnable = FALSE;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    Direct3D_GetDevice()->CreateBlendState(&blend, &g_pBlendState);

    // --- Depth stencil (disabled for 2D sprites) ---
    D3D11_DEPTH_STENCIL_DESC dsd{};
    dsd.DepthEnable = FALSE;
    Direct3D_GetDevice()->CreateDepthStencilState(&dsd, &g_pDepthStencilState);

	g_WhiteTextureID = Texture_Load(L"assets/textures/white_debug.png", false);
}

void Collision_Debug_Finalize()
{
	Texture_Release(g_WhiteTextureID);

    SAFE_RELEASE(g_pDepthStencilState);
    SAFE_RELEASE(g_pBlendState);
    SAFE_RELEASE(g_pSamplerState_Point);
    SAFE_RELEASE(g_pPSConstantBuffer0);
    SAFE_RELEASE(g_pVSConstantBuffer1);
    SAFE_RELEASE(g_pVertexBuffer);
}

void Collision_Debug_Draw(const CollisionCircle & circle, const DirectX::XMFLOAT3 & color)
{
    Shader_Begin();

    XMMATRIX mtxScale = XMMatrixScaling(circle.radius, circle.radius, 1.0f);
    XMMATRIX mtxTranslation = XMMatrixTranslation(circle.position.x, circle.position.y, 0.0f);
    XMMATRIX mtxProjection = XMMatrixOrthographicOffCenterLH(
        0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);

    XMMATRIX mtx = mtxScale * mtxTranslation * mtxProjection;
    Shader_SetMatrix(mtx);      

    XMFLOAT4X4 mtxUV;
	XMStoreFloat4x4(&mtxUV, XMMatrixIdentity());    

    // --- Colour 
    XMFLOAT4 rgba{ color.x, color.y, color.z, 1.0f };
    Direct3D_GetDeviceContext()->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &rgba, 0, 0);
    Direct3D_GetDeviceContext()->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);

    // --- Geometry ---
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

    Texture_SetTexture(g_WhiteTextureID);

    Direct3D_GetDeviceContext()->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);
    Direct3D_GetDeviceContext()->OMSetDepthStencilState(g_pDepthStencilState, 0);

    Direct3D_GetDeviceContext()->Draw(NUM_VERTEX, 0);
}

void Collision_Debug_Draw(const CollisionCapsule& capsule, const XMFLOAT3& color)
{
    // Dots along the segment, sized to the capsule's thickness.
    constexpr int STEPS = 8;
    for (int i = 0; i <= STEPS; i++)
    {
        const float t = static_cast<float>(i) / STEPS;
        const CollisionCircle c{
            { capsule.start.x + (capsule.end.x - capsule.start.x) * t,
              capsule.start.y + (capsule.end.y - capsule.start.y) * t },
            capsule.half_thickness
        };
        Collision_Debug_Draw(c, color);
    }
}
