struct VertexShaderInput
{
    float3 Position  : POSITION;
    float2 TexCoord  : TEXCOORD;
    uint InstanceID  : SV_InstanceID;
    float4 pi_offset : POSITION1;   // Per-Instance offset
    float4 pi_color  : COLOR0;      // Per-Instance color
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

ConstantBuffer<VertexTransform> TransformationCB : register(b0);

VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput OUT;

    OUT.Color = IN.pi_color;
    OUT.Position = mul(TransformationCB.MVP, float4(IN.Position + IN.pi_offset.xyz, 1.f));

    return OUT;
}
