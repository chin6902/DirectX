// Constant buffer
cbuffer ProjectionBuffer : register(b0)
{
	float4x4 projection;
};

struct VS_IN
{
	float3 position : POSITION0;
	float2 uv       : TEXCOORD0;
	float4 color    : COLOR0;
};

struct VS_OUT
{
	float4 position : SV_POSITION;
	float2 uv       : TEXCOORD0;
	float4 color    : COLOR0;
};

VS_OUT main(VS_IN input)
{
	VS_OUT output;

	output.position = mul(float4(input.position, 1.0f), projection);
	output.uv       = input.uv;
	output.color    = input.color;   // tint travels per-vertex now

	return output;
}