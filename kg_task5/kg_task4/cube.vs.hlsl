cbuffer GeomBuffer : register(b0) {
    matrix m;
    float4 size;
    float4 color;
    float4 shine;
};

cbuffer SceneBuffer : register(b1) {
    matrix vp;
    float4 cameraPos;
    int4 lightCount;
    float4 ambientColor;
    float4 dummy[20]; 
};

struct VS_IN {
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 norm : NORMAL;
    float3 tang : TANGENT;
};

struct VS_OUT {
    float4 pos : SV_POSITION;
    float4 worldPos : POSITION;
    float2 uv : TEXCOORD;
    float3 norm : NORMAL;
    float3 tang : TANGENT;
};

VS_OUT vs(VS_IN input) {
    VS_OUT o;
    float4 worldPos = mul(float4(input.pos, 1.0f), m);
    o.worldPos = worldPos;
    
    o.pos = mul(worldPos, vp);
    o.uv = input.uv;
    
    o.norm = mul(input.norm, (float3x3)m);
    o.tang = mul(input.tang, (float3x3)m);
    
    return o;
}