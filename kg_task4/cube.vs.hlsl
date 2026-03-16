cbuffer GeomBuffer : register(b0)
{
    matrix m;
    float4 size;
};
cbuffer SceneBuffer : register(b1)
{
    matrix vp;
    float4 cameraPos;
};
struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};
struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUT vs(VS_IN input)
{
    VS_OUT o;
    o.pos = mul(mul(float4(input.pos, 1.0f), m), vp);
    o.uv = input.uv;
    return o;
}
