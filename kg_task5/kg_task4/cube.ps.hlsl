Texture2D colorTexture : register(t0);
SamplerState colorSampler : register(s0);

cbuffer GeomBuffer : register(b0) {
    matrix m;
    float4 size;
    float4 color;
};

struct PS_IN {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 ps(PS_IN input) : SV_TARGET {
    float4 texColor = colorTexture.Sample(colorSampler, input.uv);
    return float4(texColor.rgb, texColor.a * color.a);
}