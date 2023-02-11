struct VertexShaderInput
{
    float3 Position : POSITION;
    float3 TexCoord : TEXCOORD;
    uint InstanceID : SV_InstanceID;
};

struct VertexShaderOutput
{
    float4 Color : COLOR;
    float4 Position : SV_Position;
};

struct VertexTransform
{
    matrix MVP;
};

struct PerInstanceData
{
    float4 offset;
    float4 color;
};

struct InstanceBuffer
{
    PerInstanceData id[32];
};

ConstantBuffer<VertexTransform> TransformationCB : register(b0);
ConstantBuffer<InstanceBuffer> InstanceCB : register(b1);

VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput OUT;

    OUT.Color = InstanceCB.id[IN.InstanceID].color;
    OUT.Position = mul(TransformationCB.MVP, float4(IN.Position + InstanceCB.id[IN.InstanceID].offset.xyz, 1.f));

    return OUT;
}
