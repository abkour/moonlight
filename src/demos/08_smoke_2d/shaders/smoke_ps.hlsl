struct PS_Input
{
    float Alpha : COLOR;
    float2 TexCoord : TEXCOORD;
    float3 Color : COLOR1;
};

Texture2D image_texture : register(t1);
SamplerState texture_sampler : register(s0);

float4 main(PS_Input IN) : SV_Target
{
    float4 image_rgba = image_texture.Sample(texture_sampler, IN.TexCoord);
    return float4(IN.Color, min(0.01f, IN.Alpha)) * image_rgba;
}