struct VertexPosColor {
	float3 Position : POSITION;
	float3 Color : COLOR;
};

struct VertexShaderOutput {
	float4 Color : COLOR;
	float4 Position : SV_Position;
};

struct Transformation {
	matrix MVP;
};

ConstantBuffer<Transformation> TransformationCB : register(b0);

VertexShaderOutput main(VertexPosColor IN) 
{
	VertexShaderOutput OUT;
	
	OUT.Position = mul(TransformationCB.MVP, float4(IN.Position, 1.f));
	OUT.Color = float4(IN.Color, 1.f);

	return OUT;
}