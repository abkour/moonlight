struct VS_Input
{
    float2 Position : POSITION;
};

struct VS_Output
{
    float4 Position : SV_Position;
};

struct ConstantData
{
    float2 scale;
    float angle;
};

ConstantBuffer<ConstantData> CB : register(b0);

VS_Output main(VS_Input IN)
{
    float cos_theta = cos(CB.angle);
    float sin_theta = sin(CB.angle);

    float2 TransformedPosition = float2(
        IN.Position.x * cos_theta - IN.Position.y * sin_theta, 
        IN.Position.x * sin_theta + IN.Position.y * cos_theta
    ); 

    VS_Output OUT;

    OUT.Position = float4(TransformedPosition * CB.scale, 0.f, 1.f);

    return OUT;
}