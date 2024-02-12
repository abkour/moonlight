struct VS_Input
{
    float2 Position : POSITION;
    float2 UV : TEXCOORD;
};

struct VS_Output
{
    float2 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

VS_Output main(VS_Input IN)
{
    VS_Output OUT;

    OUT.Position = vec4(IN.Position, 0.f, 1.f);
    OUT.UV = IN.UV;

    return OUT;
}
