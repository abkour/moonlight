struct VS_Input
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct VS_Output
{
    float4 Position : SV_Position;
};

struct VS_Constants
{
    matrix MVP;
};;

ConstantBuffer<VS_Constants> VS_CB : register(b0);

VS_Output main(VS_Input IN)
{
    VS_Output OUT;

    OUT.Position = mul(VS_CB.MVP, float4(IN.Position, 1.f));

    return OUT;
}