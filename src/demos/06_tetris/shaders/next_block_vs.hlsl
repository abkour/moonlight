static float4 Colors[] =
{
    float4(0.533, 0.988, 0.011, 1.f),
    float4(0.011, 0.988, 0.549, 1.f),
    float4(0.988, 0.011, 0.306, 1.f),
    float4(0.988, 0.368, 0.011, 1.f),
    float4(0.011, 0.451, 0.988, 1.f),
    float4(0.521, 0.345, 0.961, 1.f),
    float4(1.f, 1.f, 1.f, 1.f),
    // (quick drop color)
    float4(0.4f, 0.4f, 0.4f, 1.f)
};

struct VS_Input
{
    float2 Position : POSITION;
};

struct VS_Output
{
    float4 Color : COLOR;
    float4 Position : SV_Position;
};

struct VS_Constants
{
    matrix scale;
};

ConstantBuffer<VS_Constants> VS_Constant : register(b0);

VS_Output main(VS_Input IN)
{

}