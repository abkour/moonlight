#define INVALID_RETURN_VALUE -9999.f

struct PixelShaderInput {
	float4 Position : SV_Position;
};

struct PSState {
	float2 window_resolution;
};

ConstantBuffer<PSState> CB0 : register(b0);

static const float3 sphere_center = float3(0.f, 0.f, 0.f);
static const float light_radius = 0.1f;
static const float inner_radius = 0.4f;
static const float outer_radius = 0.8f;
static const float3 eye_position = float3(0.0f, 0.f, -outer_radius * 1.2f);

static const float3 light_position = float3(0.5f, -0.5f, 0.f);
static const float3 light_direction = normalize(-light_position);

static const float4 inner_circle_color = float4(0.f, 0.3f, 1.f, 1.f);
static const float4 outer_circle_color = float4(1.f, 1.f, 1.f, 1.f);

struct Ray {
	float3 origin;
	float3 direction;
};

Ray generate_ray(float3 origin, float3 target)
{
	Ray ray;
	ray.origin = origin;
	ray.direction = normalize(target);

	return ray;
}

float distance_from_sphere(float3 p, float3 c, float r)
{
	return length(p - c) - r;
}

float3 compute_normal(float3 p, float r)
{
	float3 c = float3(0.f, 0.f, 0.f);
	float3 ss = float3(0.001f, 0.f, 0.f);

	float gradient_x = distance_from_sphere(p + ss.xyy, c, r) - distance_from_sphere(p - ss.xyy, c, r);
	float gradient_y = distance_from_sphere(p + ss.yxy, c, r) - distance_from_sphere(p - ss.yxy, c, r);
	float gradient_z = distance_from_sphere(p + ss.yyx, c, r) - distance_from_sphere(p - ss.yyx, c, r);

	float3 normal = float3(gradient_x, gradient_y, gradient_z);

	return normalize(normal);
}

float3 compute_color(float3 p, float r)
{
	float3 normal = compute_normal(p, r);
	float3 dir_to_light = normalize(p - light_position);
	float diff_intensity = max(0.f, dot(normal, dir_to_light));

	return float3(0.f, 0.5f, 1.f) * diff_intensity;
}

float3 raymarch_sphere(Ray ray, float3 center, float radius, int maximum_steps)
{
	float e = 0.01f;
	float3 origin = ray.origin;
	for (int i = 0; i < maximum_steps; ++i) {
		float t = distance_from_sphere(origin, center, radius);
		float3 np = ray.origin + t * ray.direction;
		float dist = length(np - center);
		if (dist - e < radius) {
			return np;
		}
		origin = np;
	}

	return float3(INVALID_RETURN_VALUE, INVALID_RETURN_VALUE, INVALID_RETURN_VALUE);
}

int raymarch_through_sphere(float3 ip, float3 dir, float3 center, float radius, int maximum_steps)
{
	const float step_size = 0.02f;
	const float e = 0.01f;
	float t = 0.f;
	for (int i = 0; i < maximum_steps; ++i) {
		t += step_size;
		float3 np = ip + t * dir;
		float dist = length(np - center);
		if (dist - e > radius) {
			return i;
		}
	}

	return maximum_steps;
}

float2 compute_uv(float4 frag_pos)
{
	float aspect_ratio = CB0.window_resolution.x / CB0.window_resolution.y;
	float2 uv = frag_pos.xy / CB0.window_resolution;
	uv = uv * 2.f - 1.f;
	uv.x *= aspect_ratio;
	
	return uv;
}

float4 main(PixelShaderInput IN) : SV_Target
{
	const int maximum_steps = 10;
	const float2 uv = compute_uv(IN.Position);
	const float3 eye_target = float3(uv, 1.f);

	Ray ray = generate_ray(eye_position, eye_target);
	
	float3 outer_ip = raymarch_sphere(ray, sphere_center, outer_radius, maximum_steps);
	if (outer_ip.x != INVALID_RETURN_VALUE) {
		const int max_atm_steps = 100;
		int nsteps = raymarch_through_sphere(outer_ip, ray.direction, sphere_center, outer_radius, max_atm_steps);
		float3 delta_color = outer_circle_color.xyz / max_atm_steps;
		float3 final_color = float3(1.f, 1.f, 1.f) - delta_color * nsteps;
		return float4(final_color, 1.f);
	}


	float3 light_ip = raymarch_sphere(ray, light_position, light_radius, 10);
	if (light_ip.x != INVALID_RETURN_VALUE) {
		return float4(1.f, 1.f, 1.f, 1.f);
	}

	return float4(0.f, 0.f, 0.f, 1.f);
}