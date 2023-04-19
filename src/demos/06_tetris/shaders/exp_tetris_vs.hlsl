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
    float aspect_ratio;
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

    float blocks_per_line_x = 50.f;
    float first_block_x = 12.f;
    float first_block_y = 6.f;

    float bw = 1.f / blocks_per_line_x;
    float bh = bw * VS_Constant.aspect_ratio;

    float ox = -0.25f;
    float oy = 1.f - (bh * first_block_y);
    float dx = bw * 2.f + 0.005f;
    float dy = -dx * VS_Constant.aspect_ratio;
    float ix = instance_x * dx;
    float iy = instance_y * dy;

    float2 origin = float2(ox, oy);
    float2 displacement = float2(ix, iy);
    float2 sum = origin + displacement;
    float4 sum_v4 = float4(sum.xy, 0.f, 0.f);

    VS_Output OUT;

    //OUT.Position = mul(VS_Constant.scale, float4(IN.Position, 0.f, 1.f));
    OUT.Position = float4(bw, bh, 0.f, 1.f) * float4(IN.Position, 0.f, 1.f);
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
        OUT.Color = color;
    }

    return OUT;
}