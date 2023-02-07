struct VertexShaderInput
{
	float3 Position : POSITION;
	float3 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
	float4 Position : SV_Position;
};

struct VertexTransform
{
	matrix MVP;
};

ConstantBuffer<VertexTransform> TransformationCB : register(b0);

VertexShaderOutput main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(TransformationCB.MVP, float4(IN.Position, 1.f));

	return OUT;
}
