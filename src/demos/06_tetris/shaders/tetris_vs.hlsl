static float4 Colors[] =
{
    float4(0.533, 0.988, 0.011, 1.f),
    float4(0.011, 0.988, 0.549, 1.f),
    float4(0.988, 0.011, 0.306, 1.f),
    float4(0.85, 0.674, 0.03, 1.f),   // Orange
    float4(0.011, 0.451, 0.988, 1.f),
    float4(0.521, 0.345, 0.961, 1.f),
    float4(1.f, 1.f, 1.f, 1.f),
};

static uint4 colors_red[] =
{
    uint4(217, 26, 9, 255),
    uint4(71, 6, 0, 255),
    uint4(102, 23, 15, 255),
    uint4(161, 69, 59, 255),
    uint4(252, 191, 184, 255),
    uint4(59, 8, 2, 255),
    uint4(130, 94, 90, 255)
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

float4 convert_to_srgb(uint4 linear_scale)
{
    float4 ls = float4(linear_scale.x, linear_scale.y, linear_scale.z, linear_scale.w);
    return ls / float4(255.f, 255.f, 255.f, 255.f);
}

VS_Output main(VS_Input IN, uint InstanceID : SV_InstanceID)
{
    uint instance_x = InstanceID % 10;
    uint instance_y = InstanceID / 10;
    uint cell = InstanceData_SB[InstanceID].cell;

    float ox = -0.25f;
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
        OUT.Color = float4(0.1f, 0.1f, 0.1f, 1.f);
    } 
    else
    {
        float alpha = cell & 0x80000000 ? 0.5f : 1.f;
        float4 color = convert_to_srgb(colors_red[cell - 1]);
        color = Colors[cell - 1];
        color.xyz *= alpha;
        color = float4(0.f, 0.f, 0.f, 0.f);
        OUT.Color = color;
    }

    return OUT;
}