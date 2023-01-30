struct VertexShaderOutput
{
	float2 TexCoord : TEXCOORD;
};

Texture2D image_texture : register(t0);

SamplerState texture_sampler : register(s0);

float4 main(VertexShaderOutput IN) : SV_Target
{
	return image_texture.Sample(texture_sampler, IN.TexCoord);
}