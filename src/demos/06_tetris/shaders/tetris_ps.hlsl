struct VS_Output
{
    float4 Color : COLOR;
};

float4 main(VS_Output IN) : SV_Target
{
    return float4(0.009f, 0.f, 0.f, 0.f);
    return IN.Color;
}