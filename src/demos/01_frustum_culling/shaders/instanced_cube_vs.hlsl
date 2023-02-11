struct VertexShaderInput
{
    float3 Position : POSITION;
    float3 TexCoord : TEXCOORD;
    uint InstanceID : SV_InstanceID;
};

struct VertexShaderOutput
{
    float4 Position : SV_Position;
};

struct VertexTransform
{
    matrix MVP;
};

struct InstanceBuffer
{
    float4 offset[64];
};

ConstantBuffer<VertexTransform> TransformationCB : register(b0);
ConstantBuffer<InstanceBuffer> InstanceCB : register(b1);

VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(TransformationCB.MVP, float4(IN.Position + InstanceCB.offset[IN.InstanceID].xyz, 1.f));

    return OUT;
}
