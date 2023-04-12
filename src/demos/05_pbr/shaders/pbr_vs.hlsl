struct VS_Input
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct VS_Output
{
    float3 FragPosition : POSITION;
    float3 Normal : NORMAL;
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

    OUT.FragPosition = IN.Position;
    OUT.Normal = IN.Normal;
    OUT.Position = mul(TransformationCB.MVP, float4(IN.Position, 1.f));

    return OUT;
}