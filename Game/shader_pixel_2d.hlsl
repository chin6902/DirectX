Texture2D major_texture : register(t0);
SamplerState major_sampler : register(s0);

struct PS_IN
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

float4 main(PS_IN input) : SV_TARGET
{
    return major_texture.Sample(major_sampler, input.uv) * input.color;
}
