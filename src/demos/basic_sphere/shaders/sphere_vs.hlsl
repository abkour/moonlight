struct VertexShaderInput {
	float2 Position : POSITION;
};

struct VertexShaderOutput {
	float4 Position : SV_Position;
};

VertexShaderOutput main(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = float4(IN.Position, 0.f, 1.f);

	return OUT;
}