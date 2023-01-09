struct PixelShaderInput {
	float4 Position : SV_Position;
};

struct PSState {
	float2 window_resolution;
};

ConstantBuffer<PSState> CB0 : register(b0);

float4 main(PixelShaderInput IN) : SV_Target
{
	float radius = 0.3f;
	float aspect_ratio = CB0.window_resolution.x / CB0.window_resolution.y;
	float2 center = float2(0.f, 0.f);
	float2 uv = IN.Position.xy / CB0.window_resolution;
	uv = uv * 2.f - 1.f;
	uv.x *= aspect_ratio;
	
	float dist_to_center = sqrt(uv.x * uv.x + uv.y * uv.y);
	if (dist_to_center < radius) {
		return float4(1.f, 0.f, 0.f, 0.f);
	}

	return float4(0.f, 0.f, 0.f, 0.f);
}