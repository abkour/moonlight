struct VertexShaderOutput
{
    float4 Color : COLOR;
};

float4 main(VertexShaderOutput IN) : SV_Target
{
    return IN.Color;
}