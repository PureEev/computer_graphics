struct Light {
    float4 pos;
    float4 color;
};

cbuffer SceneBuffer : register(b0)
{
    float4x4 vp;
    float4 cameraPos;
    int4 lightCount;
    float4 ambientColor;
    Light lights[10];
    float4 frustum[6]; // Теперь смещение (offset) совпадает с C++
};

cbuffer CullParams : register(b1)
{
    uint4 numShapes; // x - количество объектов
    float4 bbMin[100];
    float4 bbMax[100];
};

RWStructuredBuffer<uint> indirectArgs : register(u0);
RWStructuredBuffer<int4> objectIds : register(u1);

bool IsBoxInside(in float4 frustum[6], in float3 bbMin, in float3 bbMax) {
    for (int i = 0; i < 6; i++) {
        const float3 norm = frustum[i].xyz;
        
        float4 p = float4(
            norm.x < 0 ? bbMin.x : bbMax.x,
            norm.y < 0 ? bbMin.y : bbMax.y,
            norm.z < 0 ? bbMin.z : bbMax.z,
            1.0f
        );
        
        float s = dot(p, frustum[i]);
        
        if (s < 0.0f) return false;
    }
    return true;
}

[numthreads(64, 1, 1)]
void cs(uint3 globalThreadId : SV_DispatchThreadID) {
    if (globalThreadId.x >= numShapes.x) {
        return;
    }

    // Проверяем видимость
    if (IsBoxInside(frustum, bbMin[globalThreadId.x].xyz, bbMax[globalThreadId.x].xyz)) {
        uint id = 0;
        // Атомарно увеличиваем счетчик видимых инстансов
        InterlockedAdd(indirectArgs[1], 1, id); 
        // Записываем индекс видимого объекта
        objectIds[id] = int4(globalThreadId.x, 0, 0, 0);
    }
}