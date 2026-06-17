// Constant buffer
cbuffer MatrixBuffer : register(b0)
{
    float4x4 mtx; // 4byte * 4
}

cbuffer UVMatrixBuffer : register(b1)
{
    float4x4 mtx_uv;
};

struct VS_INPUT
{
    float4 posL : POSITION0;
    float2 uv : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT output;
    
	output.posH = mul(input.posL, mtx);
    output.uv = mul(float4(input.uv, 0.0f, 1.0f), mtx_uv).xy;
    
	return output;
}