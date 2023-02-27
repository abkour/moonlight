struct VS_IN
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
};

struct VS_OUT
{
    float3 Normal : NORMAL;
    float3 Position : POSITION;
    float4 ClipPosition : SV_Position;
};

struct VertexTransform
{
    matrix MVP;
    matrix NormalizedMVP;
    matrix Model;
};

ConstantBuffer<VertexTransform> Transformation_CB : register(b0);

VS_OUT main(VS_IN IN)
{
    VS_OUT OUT;

    OUT.Normal = mul((float3x3)Transformation_CB.NormalizedMVP, IN.Normal);
    OUT.Position = mul(Transformation_CB.Model, IN.Position).rgb;
    OUT.ClipPosition = mul(Transformation_CB.MVP, float4(IN.Position, 1.f));

    return OUT;
}