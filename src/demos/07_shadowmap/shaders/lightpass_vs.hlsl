struct VS_Input
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct VS_Output
{
    float4 Position : SV_Position;
};

struct VertexTransform
{
    matrix LightMVP;
};

ConstantBuffer<VertexTransform> CB : register(b0);

VS_Output main(VS_Input IN)
{
    VS_Output OUT;

    OUT.Position = mul(CB.LightMVP, float4(IN.Position, 1.f));

    return OUT;
}