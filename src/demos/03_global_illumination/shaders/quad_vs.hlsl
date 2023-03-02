struct VS_IN
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VS_OUT
{
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;
};

VS_OUT main(VS_IN IN)
{
    VS_OUT OUT;

    OUT.TexCoord = IN.TexCoord;
    OUT.Position = float4(IN.Position, 0.f, 1.f);

    return OUT;
}