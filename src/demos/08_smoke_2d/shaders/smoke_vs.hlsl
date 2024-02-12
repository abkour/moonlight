struct VS_Input
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VS_Output
{
    float Alpha : COLOR;
    float2 TexCoord : TEXCOORD;
    float3 Color : COLOR1;
    float4 Position : SV_Position;
};

struct VS_Constants
{
    float aspect_ratio;
};

struct QuadTransform
{
    float alpha;
    float angle;
    float lifetime;
    float size_scale;
    float3 color;
    float2 position;
    float2 velocity;
};

ConstantBuffer<VS_Constants> ConstantData : register(b0);
StructuredBuffer<QuadTransform> QuadTransforms : register(t0);

VS_Output main( VS_Input IN, 
                uint InstanceID : SV_InstanceID)
{
    const float angle = QuadTransforms[InstanceID].angle;

    float2 Position = IN.Position 
        * QuadTransforms[InstanceID].size_scale;
        
    float newx = Position.x * cos(angle) - Position.y * sin(angle);
    float newy = Position.x * sin(angle) + Position.y * cos(angle);

    Position = float2(newx, newy) 
             + QuadTransforms[InstanceID].position;
    Position.y *= ConstantData.aspect_ratio;

    VS_Output OUT;
    
    OUT.Alpha = QuadTransforms[InstanceID].alpha;
    OUT.Color = QuadTransforms[InstanceID].color;
    OUT.Position = float4(Position, 0.f, 1.f);
    OUT.TexCoord = IN.TexCoord;

    return OUT; 
}