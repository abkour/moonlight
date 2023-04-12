struct VS_Output
{
    float4 Color : COLOR;
};

float4 main(VS_Output IN) : SV_Target
{
    return IN.Color;
}