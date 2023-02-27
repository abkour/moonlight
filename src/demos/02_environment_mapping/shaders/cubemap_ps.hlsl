struct PixelShaderInput
{
    float3 TexCoord : TEXCOORD;
};

TextureCube cube_texture : register(t0);

SamplerState texture_sampler : register(s0);

float4 main(PixelShaderInput IN) : SV_Target
{
    return cube_texture.Sample(texture_sampler, IN.TexCoord);
}