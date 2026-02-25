struct VSOutput
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

float4 ps(VSOutput input) : SV_Target
{
    return input.color;
}