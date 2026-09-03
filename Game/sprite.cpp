/*============================================================================
Contents   :  [sprite.cpp]  -  BATCHED

Author     : Chin Qing You
LastUpdate : 2026/08/20
-----------------------------------------------------------------------------

============================================================================*/
#include <algorithm>
#include <cmath>

#include "sprite.h"
#include "debug_ostream.h"
#include "config.h"
#include "direct3d.h"
#include "shader.h"
#include "texture.h"

using namespace DirectX;

// ============================================================================
// Vertex
// ============================================================================
struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT2 texcoord;
	XMFLOAT4 color;
};

static constexpr int BATCH_MAX_QUADS = 4096;
static constexpr int VERTS_PER_QUAD = 6;
static constexpr int BATCH_MAX_VERTICES = BATCH_MAX_QUADS * VERTS_PER_QUAD;

static Vertex g_Vertices[BATCH_MAX_VERTICES];
static int    g_VertexCount = 0;

// --- what the pending batch is bound to ---
static int          g_BatchTexture = TEXTURE_INVALID_ID;
static SpriteFilter g_BatchFilter = kSpriteFilter_Point;
static SpriteFilter g_PendingFilter = kSpriteFilter_Point;

static ID3D11Buffer* g_pVertexBuffer{ nullptr };
static ID3D11SamplerState* g_pSamplerState_Point{ nullptr };
static ID3D11SamplerState* g_pSamplerState_Linear{ nullptr };
static ID3D11BlendState* g_pBlendState{ nullptr };
static ID3D11DepthStencilState* g_pDepthStencilState{ nullptr };
static ID3D11RasterizerState* g_pRasterizerState{ nullptr };

// ============================================================================
// Initialize
// ============================================================================
bool Sprite_Initialize()
{
	// --- DYNAMIC vertex buffer, rewritten every frame ---
	{
		D3D11_BUFFER_DESC bd{
			.ByteWidth = sizeof(Vertex) * BATCH_MAX_VERTICES,
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
		};

		if (FAILED(Direct3D_GetDevice()->CreateBuffer(&bd, nullptr, &g_pVertexBuffer)))
		{
			hal::dout << "Sprite_Initialize(): 頂点バッファの作成に失敗しました" << std::endl;
			return false;
		}
	}

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

	// --- Blend state ---
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

	// --- Depth stencil ---
	D3D11_DEPTH_STENCIL_DESC dsd{};
	dsd.DepthEnable = FALSE;
	Direct3D_GetDevice()->CreateDepthStencilState(&dsd, &g_pDepthStencilState);

	// --- Rasterizer ---
	D3D11_RASTERIZER_DESC rd{};
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_NONE;
	Direct3D_GetDevice()->CreateRasterizerState(&rd, &g_pRasterizerState);
	Direct3D_GetDeviceContext()->RSSetState(g_pRasterizerState);

	g_VertexCount = 0;
	g_BatchTexture = TEXTURE_INVALID_ID;
	g_BatchFilter = kSpriteFilter_Point;
	g_PendingFilter = kSpriteFilter_Point;
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
	SAFE_RELEASE(g_pVertexBuffer);
}

// ============================================================================
// Flush
// ============================================================================
void Sprite_Flush()
{
	if (g_VertexCount == 0 || g_BatchTexture == TEXTURE_INVALID_ID)
	{
		g_VertexCount = 0;
		return;
	}

	ID3D11DeviceContext* ctx = Direct3D_GetDeviceContext();

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (SUCCEEDED(ctx->Map(g_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		memcpy(mapped.pData, g_Vertices, sizeof(Vertex) * g_VertexCount);
		ctx->Unmap(g_pVertexBuffer, 0);
	}

	Shader_Begin();

	Shader_SetMatrix(XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	ID3D11SamplerState* sampler =(g_BatchFilter == kSpriteFilter_Linear) ? g_pSamplerState_Linear : g_pSamplerState_Point;
	ctx->PSSetSamplers(0, 1, &sampler);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	ctx->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
	ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	Texture_SetTexture(g_BatchTexture);

	ctx->OMSetBlendState(g_pBlendState, nullptr, 0xffffffff);
	ctx->OMSetDepthStencilState(g_pDepthStencilState, 0);

	ctx->Draw(g_VertexCount, 0);    

	g_VertexCount = 0;
}

void Sprite_SetFilter(SpriteFilter filter)
{
	g_PendingFilter = filter;
}

// ============================================================================
// Internal draw
// ============================================================================
static void Sprite_DrawInternal(
	int   texture_id,
	float x, float y,
	float width, float height,
	float texture_x, float texture_y,
	int   texture_width, int texture_height,
	const SpriteDrawParams& params)
{
	if (texture_id == TEXTURE_INVALID_ID) { return; }

	const unsigned int tex_w = Texture_GetWidth(texture_id);
	const unsigned int tex_h = Texture_GetHeight(texture_id);
	if (tex_w == 0 || tex_h == 0) { return; }

	// --- state change ends the current batch ---
	if (texture_id != g_BatchTexture
		|| g_PendingFilter != g_BatchFilter
		|| g_VertexCount + VERTS_PER_QUAD > BATCH_MAX_VERTICES)
	{
		Sprite_Flush();
		g_BatchTexture = texture_id;
		g_BatchFilter = g_PendingFilter;
	}

	// --- geometry ---
	const float scaleX = params.flip_x ? -(params.scale.x) : params.scale.x;
	const float scaleY = params.flip_y ? -(params.scale.y) : params.scale.y;

	const float hw = width * scaleX * 0.5f;
	const float hh = height * scaleY * 0.5f;

	const float cx = x + width * params.scale.x * 0.5f;
	const float cy = y + height * params.scale.y * 0.5f;

	XMFLOAT2 corner[4] =
	{
		{ -hw, -hh },   // 0 top-left
		{  hw, -hh },   // 1 top-right
		{ -hw,  hh },   // 2 bottom-left
		{  hw,  hh },   // 3 bottom-right
	};

	if (params.angle != 0.0f)
	{
		const float s = sinf(params.angle);
		const float c = cosf(params.angle);

		for (XMFLOAT2& v : corner)
		{
			const float rx = v.x * c - v.y * s;
			const float ry = v.x * s + v.y * c;
			v = { rx, ry };
		}
	}

	for (XMFLOAT2& v : corner) { v = { v.x + cx, v.y + cy }; }

	// --- UVs ---
	const float tx = texture_x / static_cast<float>(tex_w);
	const float ty = texture_y / static_cast<float>(tex_h);
	const float tw = texture_width / static_cast<float>(tex_w);
	const float th = texture_height / static_cast<float>(tex_h);

	const XMFLOAT2 uv[4] =
	{
		{ tx,      ty      },
		{ tx + tw, ty      },
		{ tx,      ty + th },
		{ tx + tw, ty + th },
	};

	const XMFLOAT4 rgba{ params.color.x, params.color.y, params.color.z, params.alpha };

	static constexpr int order[VERTS_PER_QUAD] = { 0, 1, 2,  1, 3, 2 };

	for (int i = 0; i < VERTS_PER_QUAD; i++)
	{
		const int cidx = order[i];
		Vertex& v = g_Vertices[g_VertexCount++];

		v.position = { corner[cidx].x, corner[cidx].y, 0.0f };
		v.texcoord = uv[cidx];
		v.color = rgba;
	}
}

static const SpriteDrawParams DEFAULT_PARAMS{};

// ============================================================================
// Public overloads
// ============================================================================
void Sprite_Draw(int texture_id, float x, float y)
{
	Sprite_Draw(texture_id, x, y, DEFAULT_PARAMS);
}

void Sprite_Draw(int texture_id, float x, float y, const SpriteDrawParams& params)
{
	const float w = static_cast<float>(Texture_GetWidth(texture_id));
	const float h = static_cast<float>(Texture_GetHeight(texture_id));

	Sprite_DrawInternal(texture_id, x, y, w, h, 0.0f, 0.0f,
		Texture_GetWidth(texture_id), Texture_GetHeight(texture_id), params);
}

void Sprite_Draw(int texture_id, float x, float y, float width, float height)
{
	Sprite_Draw(texture_id, x, y, width, height, DEFAULT_PARAMS);
}

void Sprite_Draw(int texture_id, float x, float y,
	float width, float height, const SpriteDrawParams& params)
{
	Sprite_DrawInternal(texture_id, x, y, width, height, 0.0f, 0.0f,
		Texture_GetWidth(texture_id), Texture_GetHeight(texture_id), params);
}

void Sprite_Draw(int texture_id, float x, float y,
	float width, float height,
	float texture_x, float texture_y,
	int texture_width, int texture_height)
{
	Sprite_Draw(texture_id, x, y, width, height,
		texture_x, texture_y, texture_width, texture_height, DEFAULT_PARAMS);
}

void Sprite_Draw(int texture_id, float x, float y,
	float width, float height,
	float texture_x, float texture_y,
	int texture_width, int texture_height,
	const SpriteDrawParams& params)
{
	Sprite_DrawInternal(texture_id, x, y, width, height,
		texture_x, texture_y, texture_width, texture_height, params);
}