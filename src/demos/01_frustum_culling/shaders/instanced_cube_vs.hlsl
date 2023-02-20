struct VertexShaderInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
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

struct InstanceIDStructure
{
    uint Index;
};

struct InstanceFormat
{
    float4 Displacement;
    float4 Color;
};

ConstantBuffer<VertexTransform> Transformation_CB : register(b0);
StructuredBuffer<InstanceIDStructure> InstanceID_SB : register(t0);
StructuredBuffer<InstanceFormat> InstanceData_SB : register(t1);

VertexShaderOutput main(
    VertexShaderInput IN, 
    uint InstanceID : SV_InstanceID)
{
    VertexShaderOutput OUT;

    uint Idx = InstanceID_SB[InstanceID].Index;

    OUT.Color = InstanceData_SB[Idx].Color;
    OUT.Position = mul(Transformation_CB.MVP, 
        float4(IN.Position + InstanceData_SB[Idx].Displacement.xyz, 1.f));

    return OUT;
}
