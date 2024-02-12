struct VS_Input
{
    float2 Position : POSITION;
    float2 UV : TEXCOORD;
};

struct VS_Output
{
    float4 Position : SV_Position;
};

VS_Output main(VS_Input IN)
{
    VS_Output OUT;

    OUT.Position = float4(IN.Position, 0.f, 1.f);

    return OUT;
}