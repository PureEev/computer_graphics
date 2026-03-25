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
};
struct VS_OUT
{
    float4 pos : SV_POSITION;
    float3 localPos : POSITION1;
};

VS_OUT vs(VS_IN input)
{
    VS_OUT result;
    float3 pos = cameraPos.xyz + input.pos * size.x;
    result.pos = mul(float4(pos, 1.0), vp).xyww; // Оптимизация z=w
    result.localPos = input.pos;
    return result;
}