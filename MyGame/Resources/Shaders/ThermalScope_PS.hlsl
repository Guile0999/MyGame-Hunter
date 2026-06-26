Texture2D glowTex : register(t0);
SamplerState samp : register(s0);

cbuffer ThermalCB : register(b0)
{
    float2 glowCenter;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    // Map screen-space texcoord (0..1)
    float2 uv = input.Tex;

    // Offset by glow center
    float2 offsetUV = uv - glowCenter;

    // Optional: scale / rotate / animate
    float2 finalUV = offsetUV + glowCenter;

    float4 color = glowTex.Sample(samp, finalUV);

    color.rgb *= float3(1.0f, 0.95f, 0.4f); // HOT yellow tint
    return color;
}