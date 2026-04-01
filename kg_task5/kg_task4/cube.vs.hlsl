cbuffer GeomBufferInst : register(b0) {
    matrix m[100];
    float4 size[100];
    float4 colorInst[100];
    float4 shineInst[100];
};

cbuffer SceneBuffer : register(b1) {
    matrix vp;
    float4 cameraPos;
    int4 lightCount;
    float4 ambientColor;
    float4 dummy[20];
};

// Буфер с ИНДЕКСАМИ видимых кубов (из Culling)
cbuffer GeomBufferInstVis : register(b2) {
    uint4 ids[100];
};

struct VS_IN {
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 norm : NORMAL;
    float3 tang : TANGENT;
    uint instanceId : SV_InstanceID; // Системная переменная Инстансинга
};

struct VS_OUT {
    float4 pos : SV_POSITION;
    float4 worldPos : POSITION;
    float2 uv : TEXCOORD;
    float3 norm : NORMAL;
    float3 tang : TANGENT;
    nointerpolation uint instanceId : INST_ID; // Пробрасываем индекс в пиксельный шейдер
};

VS_OUT vs(VS_IN input) {
    VS_OUT o;
    // Берем ID текущего видимого куба
    uint idx = ids[input.instanceId].x;

    float4 worldPos = mul(float4(input.pos, 1.0f), m[idx]);
    o.worldPos = worldPos;
    o.pos = mul(worldPos, vp);
    o.uv = input.uv;

    o.norm = mul(input.norm, (float3x3)m[idx]);
    o.tang = mul(input.tang, (float3x3)m[idx]);
    o.instanceId = idx;

    return o;
}