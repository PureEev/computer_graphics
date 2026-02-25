struct VSInput
{
    float3 pos   : POSITION;
    float4 color : COLOR;
};

struct VSOutput
{
    float4 pos   : SV_Position;
    float4 color : COLOR;
};

VSOutput vs(VSInput input)
{
    VSOutput output;
    output.pos = float4(input.pos, 1.0);
    output.color = input.color;
    return output;
}