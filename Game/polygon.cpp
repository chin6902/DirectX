/*============================================================================
Contents   :  [polygon.cpp]
              
Author     : Chin Qing You
LastUpdate : 2026/06/08
-----------------------------------------------------------------------------

============================================================================*/
#include <d3d11.h>
#include <DirectXMath.h>

#include "debug_ostream.h"
#include "config.h"
#include "polygon.h"
#include "direct3d.h"
#include "shader.h"

using namespace DirectX;

static ID3D11Buffer* g_pVertexBuffer = nullptr;

// 頂点構造体
struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

static constexpr int NUM_VERTEX { 6 };

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

    return true;
}

void Polygon_Finalize()
{
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
    v[1].position = { 500.0f, 100.0f, 0.0f };
    v[1].color = { 0.0f, 1.0f, 0.0f, 1.0f }; // 緑  
    v[2].position = { 100.0f, 500.0f, 0.0f };
    v[2].color = { 0.0f, 0.0f, 1.0f, 1.0f }; // 青
    v[3].position = { 500.0f, 500.0f, 0.0f };
    v[3].color = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色
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

    Direct3D_GetDeviceContext()->Draw(4, 0);
}
