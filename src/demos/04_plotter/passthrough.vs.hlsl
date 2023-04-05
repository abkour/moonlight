struct VS_Input
{
    float3 Position : POSITION;
};

struct VS_Output
{
    float4 Position : SV_Position;
};

struct TransformCB
{
    float4x4 MVP;
};

ConstantBuffer<TransformCB> TCB : register(b0);

VS_Output main(VS_Input IN)
{
    VS_Output OUT;

    OUT.Position = mul(TCB.MVP, float4(IN.Position, 1.f));

    return OUT;
}