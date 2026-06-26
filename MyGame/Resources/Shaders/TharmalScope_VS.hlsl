struct VS_OUT
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

VS_OUT main(uint id : SV_VertexID)
{
    float2 pos[4] =
    {
        float2(-1, -1),
        float2(-1, 1),
        float2(1, -1),
        float2(1, 1)
    };

    float2 uv[4] =
    {
        float2(0, 1),
        float2(0, 0),
        float2(1, 1),
        float2(1, 0)
    };

    VS_OUT o;
    o.Pos = float4(pos[id], 0, 1);
    o.UV = uv[id];
    return o;
}