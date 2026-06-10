// 2D vertex shader
float4x4 mtx; 

struct VS_INPUT
{
    float4 posL : POSITION0;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
    float2 uv : TEXCOORD0;
};

VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT output;
	output.posH = mul(input.posL, mtx);
	output.color = input.color;
	output.uv = input.uv;
	return output;
}