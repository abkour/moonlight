struct VS_IN
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct VS_OUT
{
    float4 Position : SV_Position;
};

VS_OUT main(VS_IN IN)
{
    VS_OUT OUT;

    OUT.Position = float4(IN.Position, 1.f);

    return OUT;
}