struct PS_IN
{
    float2 TexCoord : TEXCOORD;
};

Texture2D scene_texture : register(t0);
SamplerState tex_sampler : register(s0);

float4 main(PS_IN IN) : SV_Target
{
    return scene_texture.Sample(tex_sampler, IN.TexCoord);
}