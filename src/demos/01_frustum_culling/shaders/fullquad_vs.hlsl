struct VertexShaderInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float2 TexCoord : TEXCOORD;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexShaderInput IN)
{
    VertexShaderOutput OUT;

    OUT.TexCoord = IN.TexCoord;
    OUT.Position = float4(IN.Position, 1.f);

    return OUT;
}