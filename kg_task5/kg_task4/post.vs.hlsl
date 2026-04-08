struct VSOutput {
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
};

VSOutput vs(uint vertexId : SV_VertexID) {
    VSOutput result;
    // Магия создания треугольника, закрывающего весь экран
    float2 texcoord = float2((vertexId << 1) & 2, vertexId & 2);
    result.pos = float4(texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
    result.uv = texcoord;
    return result;
}