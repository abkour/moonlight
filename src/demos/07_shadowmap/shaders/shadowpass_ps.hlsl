struct PS_Input
{
    float4 Position : SV_Position;
    float4 LightFragPosition : POSITION;
};

Texture2D scene_texture : register(t0);
SamplerState tsampler : register(s0);

float4 shadow_computation(in float4 lightPosFragment)
{
    float3 lightfrag_proj = lightPosFragment.xyz;
    lightfrag_proj.xy = lightfrag_proj.xy * 0.5 + 0.5;
    float shadowmap_depth = scene_texture.Sample(tsampler, lightfrag_proj.xy).r;

    if(lightfrag_proj.x < 0.f || lightfrag_proj.x > 1.f)
    {
        return float4(1.f, 0.f, 0.f, 1.f);
    }
    if(lightfrag_proj.y < 0.f || lightfrag_proj.y > 1.f)
    {
        return float4(1.f, 1.f, 0.f, 1.f);
    }
    if(lightfrag_proj.z < 0.f || lightfrag_proj.z > 1.f)
    {
        return float4(0.f, 1.f, 1.f, 1.f);
    }

    float z = lightfrag_proj.z;
    float4 zz = float4(z, z, z, 1.f);
    float4 pp = float4(shadowmap_depth, shadowmap_depth, shadowmap_depth, 1.f);
    float4 tt = float4(lightfrag_proj.y, 0.f, 0.f, 0.f);

    return pp;

    float is_shadow = (shadowmap_depth < lightfrag_proj.z) ? 0.f : 1.f;
    return float4(is_shadow, is_shadow, is_shadow, is_shadow);
}

float4 main(PS_Input IN) : SV_Target
{
    float4 in_shadow = shadow_computation(IN.LightFragPosition);
    return float4(1.f, 1.f, 1.f, 1.f) * float4(in_shadow.xyz, 1.f);
}