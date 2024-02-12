struct VS_Input
{
    float2 Position : POSITION;
};

struct VS_Output
{
    float3 Color : COLOR0;
    float4 Position : SV_Position;
};

struct VS_Constants_CB
{
    // This variable in effect scales the 'Offset' variable in the 
    // per instance struct to the range [-1,-1] x [1,1].
    // The 'Position' variable for the vertices is already in that range.
    float2 inv_scale;
};

struct InstanceData_SB
{
    float2 Offset;
};

struct InstanceData_Color_SB
{
    float3 Color;
};

ConstantBuffer<VS_Constants_CB> ConstantData : register(b0);
StructuredBuffer<InstanceData_SB> InstanceData : register(t0);
StructuredBuffer<InstanceData_Color_SB> InstanceColor : register(t1);

VS_Output main(VS_Input IN, 
               uint InstanceID : SV_InstanceID)
{
    float aspect_ratio = 1.f;
    float2 adj_offset = InstanceData[InstanceID].Offset * ConstantData.inv_scale;
    adj_offset = adj_offset * 2.f - 1.f;

    VS_Output OUT;

    OUT.Color = InstanceColor[InstanceID].Color;
    OUT.Position = float4(
        IN.Position.x + adj_offset.x, 
        IN.Position.y * aspect_ratio + adj_offset.y, 
        0.f, 
        1.f
    );

    return OUT;
}