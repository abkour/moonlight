struct VertexShaderOutput
{
    float2 TexCoord : TEXCOORD;
};

Texture2D scene_texture : register(t0);

SamplerState tsampler : register(s0);

float4 main(VertexShaderOutput IN) : SV_Target
{
    return scene_texture.Sample(tsampler, IN.TexCoord);
}