struct PS_IN
{
    float3 Normal : NORMAL;
    float3 Position : POSITION;
};

struct PS_OUT 
{
    float4 Color : SV_Target;
};

struct PS_Constants
{
    float3 camera_pos;
};

ConstantBuffer<PS_Constants> ps_constants : register(b0);

TextureCube cube_texture : register(t0);
SamplerState texture_sampler : register(s0);

PS_OUT main(PS_IN IN)
{
    PS_OUT OUT;

    float3 I = normalize(IN.Position - ps_constants.camera_pos);
    float3 R = reflect(I, normalize(IN.Normal));

    float4 sky_color = cube_texture.Sample(texture_sampler, R);
    OUT.Color = float4(sky_color.rgb, 1.f);

    return OUT;
}