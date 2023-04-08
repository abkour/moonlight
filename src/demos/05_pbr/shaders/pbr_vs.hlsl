struct VS_Input
{
    float3 Position : POSITION;
};

struct VS_Output
{
    float4 Position : SV_Position;
};

struct TransformationStruct
{
    matrix MVP;
};

ConstantBuffer<TransformationStruct> TransformationCB : register(b0);

VS_Output main(VS_Input IN)
{
    VS_Output OUT;

    OUT.Position = mul(TransformationCB.MVP, float4(IN.Position, 1.f));

    return OUT;
}