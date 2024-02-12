struct PS_Input
{
    float3 Color : COLOR0;
};

float4 main(PS_Input IN) : SV_target
{
    return float4(IN.Color, 1.f);
}