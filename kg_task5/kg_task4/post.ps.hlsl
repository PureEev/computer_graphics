Texture2D colorTexture : register(t0);
SamplerState colorSampler : register(s0);

struct PS_IN {
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

float4 ps(PS_IN input) : SV_Target{
    // Берем оригинальный цвет пикселя
    float3 color = colorTexture.Sample(colorSampler, input.uv).xyz;

// Формула Сепии
float r = dot(color, float3(0.393f, 0.769f, 0.189f));
float g = dot(color, float3(0.349f, 0.686f, 0.168f));
float b = dot(color, float3(0.272f, 0.534f, 0.131f));

return float4(r, g, b, 1.0f);
}