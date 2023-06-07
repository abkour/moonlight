struct VS_Input
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    uint VertexID : SV_VertexID;
};

struct VS_Output
{
    float4 Position : SV_Position;
    float4 LightFragPosition : POSITION;
};

struct VertexTransform
{
    matrix MVP;
};

ConstantBuffer<VertexTransform> CameraCB : register(b0);
ConstantBuffer<VertexTransform> LightCB : register(b1);

VS_Output main(VS_Input IN)
{
    VS_Output OUT;

    OUT.Position = mul(CameraCB.MVP, float4(IN.Position, 1.f));
    OUT.LightFragPosition = mul(LightCB.MVP, float4(IN.Position, 1.f));

    return OUT;
}