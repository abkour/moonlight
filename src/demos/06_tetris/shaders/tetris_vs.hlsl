static float4 Colors[] =
{
    float4(1.f, 0.f, 0.f, 1.f),
    float4(0.f, 1.f, 0.f, 1.f),
    float4(0.f, 0.f, 1.f, 1.f),
    float4(1.f, 1.f, 0.f, 1.f),
    float4(0.f, 1.f, 1.f, 1.f),
    float4(1.f, 1.f, 1.f, 1.f),
    float4(1.f, 0.6f, 0.2f, 1.f),
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

struct InstanceFormat
{
    uint cell;
};

ConstantBuffer<VS_Constants> VS_Constant : register(b0);
StructuredBuffer<InstanceFormat> InstanceData_SB : register(t0);

VS_Output main(VS_Input IN, uint InstanceID : SV_InstanceID)
{
    uint instance_x = InstanceID % 10;
    uint instance_y = InstanceID / 10;
    uint cell = InstanceData_SB[InstanceID].cell;

    float ox = -0.5f;
    float oy = 0.0f;
    float dx = 1.f / 36.f;
    float dy = -1.f / 18.f;
    float ix = instance_x * dx;
    float iy = instance_y * dy;

    float2 origin = float2(ox, oy);
    float2 displacement = float2(ix, iy);
    float2 sum = origin + displacement;
    float4 sum_v4 = float4(sum.xy, 0.f, 0.f);

    VS_Output OUT;

    OUT.Position = mul(VS_Constant.scale, float4(IN.Position, 0.f, 1.f));
    OUT.Position = OUT.Position + sum_v4;
    if (cell == 0)
    {
        OUT.Position.z = 255;
    } 
    else
    {
        OUT.Color = Colors[cell - 1];
    }

    return OUT;
}