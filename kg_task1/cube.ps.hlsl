struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

float4 ps(PS_IN input) : SV_TARGET
{
    return input.col;
}