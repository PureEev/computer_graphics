Texture2D colorTexture : register(t0);
SamplerState colorSampler : register(s0);
struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 ps(PS_IN input) : SV_TARGET
{
    return colorTexture.Sample(colorSampler, input.uv);
}
