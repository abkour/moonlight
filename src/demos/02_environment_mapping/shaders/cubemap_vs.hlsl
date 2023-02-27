struct VertexInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct VertexShaderOutput
{
    float3 TexCoord : TEXCOORD;
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
    
    OUT.TexCoord = IN.Position;
    OUT.Position = mul(Transformation_CB.MVP, float4(IN.Position, 1.f));
    OUT.Position = OUT.Position.xyww;

    return OUT;
}