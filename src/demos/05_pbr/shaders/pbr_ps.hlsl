#define Pi 3.1415926f

#define Water 0
#define Plastic 1
#define PlasticHigh 2
#define Glass 3
#define Diamond 4
#define Iron 5
#define Copper 6
#define Gold 7
#define Aluminium 8
#define Silver 9

// Specular term
static float3 f0_values[] =
{
    /* Water */         float3(0.15f, 0.15f, 0.15f),
    /* Plastic */       float3(0.21f, 0.21f, 0.21f),
    /* Plastic High */  float3(0.24f, 0.24f, 0.24f),
    /* Glass */         float3(0.31f, 0.31f, 0.31f),
    /* Diamond */       float3(0.45f, 0.45f, 0.45f),
    /* Iron */          float3(0.77f, 0.78f, 0.78f),
    /* Copper */        float3(0.98f, 0.82f, 0.76f),
    /* Gold */          float3(1.00f, 0.86f, 0.57f),
    /* Aluminium */     float3(0.96f, 0.96f, 0.97f),
    /* Silver */        float3(0.98f, 0.97f, 0.95f),
};

struct PS_Input
{
    float3 FragPosition : POSITION;
    float3 Normal : NORMAL;
};

struct PSConstants 
{
    float4 light_position;
    float4 light_direction;
    float4 light_luminance;
    float4 view_position;
    float4 view_direction;
    // New
    float4 albedo;
    float metallic;
    float roughness;
    float ao;
};

struct PixelConstants
{
    PSConstants psc;
};

ConstantBuffer<PixelConstants> PSConstantObject : register(b0);

// Blinn-Phong
float3 blinn_phong_shading(in PS_Input IN);

// Diffuse term
float3 diffuse_brdf(in float3 object_color, in float3 v, in float3 l);

float spec_ndf_ggx(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float dot_nh = max(dot(N, H), 0.0);

    float nom = a2;
    float denom = (dot_nh * dot_nh * (a2 - 1.0) + 1.0);
    denom = Pi * denom * denom;

    return nom / denom;
}

// ----------------------------------------------------------------------------
float spec_geometry_schlick_ggx(float dot_nv, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = dot_nv;
    float denom = dot_nv * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float spec_geometry_smith(float3 N, float3 V, float3 L, float roughness)
{
    float dot_nl = max(dot(N, L), 0.0);
    float dot_nv = max(dot(N, V), 0.0);
    float ggx1 = spec_geometry_schlick_ggx(dot_nl, roughness);
    float ggx2 = spec_geometry_schlick_ggx(dot_nv, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
float3 spec_fresnel_schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 spec_fresnel_schlick_sg(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(2.f, (-5.55473 * cosTheta - 6.98316) * cosTheta);
}

float3 compute_brdf(PS_Input IN)
{
    PSConstants psc = PSConstantObject.psc;

    float3 N = IN.Normal;
    float3 V = normalize(psc.view_position.xyz - IN.FragPosition);

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, psc.albedo.xyz, psc.metallic);

    float3 Lo = float3(0.0, 0.0, 0.0);
    
    // Integrate Lo
    float3 L = normalize(psc.light_position.xyz - IN.FragPosition);
    float3 H = normalize(L + V);
    float distance    = length(psc.light_position.xyz - IN.FragPosition);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance   = psc.light_luminance.xyz * attenuation;

    float  NDF = spec_ndf_ggx(N, H, psc.roughness);
    float  G   = spec_geometry_smith(N, V, L, psc.roughness);
    float3 F   = spec_fresnel_schlick_sg(clamp(dot(H, V), 0.f, 1.f), F0);

    float3 numerator  = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.f) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular   = numerator / denominator;

    float3 kS = F;
    float3 kD = float3(1.f, 1.f, 1.f) - kS;
    kD *= 1.f - psc.metallic;

    float dot_nl = max(dot(N, L), 0.0);

    Lo += (kD * psc.albedo.xyz / Pi + specular) * radiance * dot_nl;
    // Integration over

    float3 ambient = float3(0.03, 0.03, 0.03) * psc.albedo.xyz * psc.ao;
    
    float3 color = ambient + Lo;
    color = color / (color + float3(1.f, 1.f, 1.f));
    float exp = 1.0 / 2.2;
    color = pow(color, float3(exp, exp, exp));

    return color;
}

float4 main(PS_Input IN) : SV_Target
{
    //float3 color = blinn_phong_shading(IN);
    float3 color = compute_brdf(IN);
    return float4(color, 1.f);
}

float3 blinn_phong_shading(in PS_Input IN)
{
    float3 light_color = float3(1.f, 1.f, 1.f);
    float3 object_color = float3(1.f, 0.6f, 0.2f);

    PSConstants psc = PSConstantObject.psc;
    float3 light_dir = normalize(psc.light_position.xyz - IN.FragPosition);

    float diffuse_scale = max(0.f, dot(light_dir, IN.Normal));
    float3 diffuse_color = light_color * diffuse_scale;

    float specular_strength = 0.5;
    float3 view_dir = normalize(psc.view_position.xyz - IN.FragPosition);
    float3 reflect_dir = reflect(-light_dir, IN.Normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    float3 specular_color = specular_strength * spec * light_color;

    float3 color = (specular_color + diffuse_color) * object_color;
    return color;
}

float3 diffuse_brdf(in float3 object_color, in float3 v, in float3 l)
{
    return object_color / Pi;
}
