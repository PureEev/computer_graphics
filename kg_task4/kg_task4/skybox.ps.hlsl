TextureCube skyboxTexture : register(t0);
SamplerState colorSampler : register(s0);
struct VS_OUT
{
    float4 pos : SV_POSITION;
    float3 localPos : POSITION1;
};

float4 ps(VS_OUT input) : SV_TARGET
{
    return float4(skyboxTexture.Sample(colorSampler, input.localPos).xyz, 1.0);
}
