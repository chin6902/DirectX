// Constant buffer
cbuffer ColorBuffer : register(b0)
{
    float4 color; 
}

struct PS_INPUT
{
    float4 posH : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D major_texture : register(t0); // テクスチャ
SamplerState major_sampler : register(s0); // サンプラー

float4 main(PS_INPUT input) : SV_TARGET
{
	return major_texture.Sample(major_sampler, input.uv) * color;
}

