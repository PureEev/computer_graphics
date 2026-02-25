cbuffer GeomBuffer : register(b0)
{
	matrix m;
};

cbuffer SceneBuffer : register(b1)
{
	matrix vp;
};

struct VS_IN
{
	float3 pos : POSITION;
	float4 col : COLOR;
};

struct VS_OUT
{
	float4 pos : SV_POSITION;
	float4 col : COLOR;
};

VS_OUT vs(VS_IN input)
{
	VS_OUT o;
	float4 w = mul(float4(input.pos, 1), m);
	o.pos = mul(w, vp);
	o.col = input.col;
	return o;
}