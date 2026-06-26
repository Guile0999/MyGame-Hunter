cbuffer ConstBuffer	: register(b0)
{
	float2 windowSize;
    float alphaData;
    float dummy;
};

struct VS_INPUT
{
	float3 pos : POSITION;
	float4 color: COLOR;
	float2 tex : TEXCOORD;
};

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 color: COLOR;
	float2 tex : TEXCOORD;
};