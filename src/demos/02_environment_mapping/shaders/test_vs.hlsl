struct VertexInput
{
    float3 Position : POSITION;
};

struct VertexShaderOutput
{
    float4 Position : SV_Position;
};

struct VertexTransform
{
    matrix MVP;
};

ConstantBuffer<VertexTransform> Transformation_CB : register(b0);

VertexShaderOutput main(VertexInput IN)
{
    VertexShaderOutput OUT;
    
    OUT.Position = float4(IN.Position, 1.f);

    return OUT;
}