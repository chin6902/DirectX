// 2D vertex shader
float4x4 mtx; 

struct VS_INPUT
{
    float4 posL : POSITION0;
    float4 color : COLOR0;
};

struct VS_OUTPUT
{
    float4 posH : SV_POSITION;
    float4 color : COLOR0;
};

VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT output;
	output.posH = mul(input.posL, mtx);
	output.color = input.color;
	return output;
}