struct VertexAttributes {
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput {
	float2 TexCoord : TEXCOORD;
	float4 Position : SV_Position;
};

struct VertexTransformation
{
	matrix MVP;
};

ConstantBuffer<VertexTransformation> TransformationCB : register(b0);

VertexShaderOutput main(VertexAttributes IN)
{
	VertexShaderOutput OUT;
	
	OUT.TexCoord = IN.TexCoord;
	OUT.Position = mul(TransformationCB.MVP, float4(IN.Position, 1.f));

	return OUT;
}