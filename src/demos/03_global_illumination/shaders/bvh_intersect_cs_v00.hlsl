#define BLOCK_SIZE          8
#define EPSILON             0.0000001   // 1e-7
#define FLOAT32_INVALID     1073741824  // 2^30
#define VERT_PER_TRIANGLE   3

struct CS_Input
{
    uint3 GroupID           : SV_GroupID;           // 3D index of the thread group in the dispatch.
    uint3 GroupThreadID     : SV_GroupThreadID;     // 3D index of local thread ID in a thread group.
    uint3 DispatchThreadID  : SV_DispatchThreadID;  // 3D index of global thread ID in the dispatch.
    uint GroupIndex         : SV_GroupIndex;        // Flattened local index of the thread within a thread group.
};

struct TriangleStrideStructure
{
    uint stride;
};

struct TriangleIndexFormat
{
    uint v;
};

struct TriangleFormat
{
    float v;
};

struct BVHNode
{
    float3 aabbmin; 
    unsigned int left_first;
    float3 aabbmax; 
    unsigned int tri_count;
};

struct Ray
{
    float3 o, d, invd;
    float t;
};

struct RayCameraStructure
{
    uint2 resolution;
    float3 eyepos, eyedir;
    float3 shiftx, shifty, topLeftPixel;
};

ConstantBuffer<TriangleStrideStructure> triangle_stride_struct : register(b0);
ConstantBuffer<RayCameraStructure> RayCamera : register(b1);
StructuredBuffer<BVHNode> BVHNodes : register(t0);
StructuredBuffer<TriangleFormat> Triangles : register(t1);
StructuredBuffer<TriangleIndexFormat> TriangleIndices : register(t2);

// Output RWTexture2D
RWTexture2D<float4> output_texture : register(u0);

Ray generate_ray(uint x, uint y)
{
    float3 direction = 
        RayCamera.topLeftPixel +
        (RayCamera.shiftx * (float(x) - 1.f)) +
        (RayCamera.shifty * (float(y) - 1.f));

    Ray ray;
    ray.o = RayCamera.eyepos;
    ray.d = normalize(direction);
    ray.invd.x = 1.f / ray.d.x;
    ray.invd.y = 1.f / ray.d.y;
    ray.invd.z = 1.f / ray.d.z;
    ray.t = FLOAT32_INVALID;

    return ray;
}

uint compute_triangle_pos(
    in uint triangle_pos, in uint stride)
{
    return TriangleIndices[triangle_pos].v
        * (stride * VERT_PER_TRIANGLE + VERT_PER_TRIANGLE);
}

void swap_float(inout float x, inout float y)
{
    float tmp = x;
    x = y;
    y = tmp;
}

void swap_uint(inout uint x, inout uint y)
{
    uint tmp = x;
    x = y;
    y = tmp;
}

float ray_hit_triangle(
    Ray ray,
    uint triangle_pos,
    uint stride)
{
    float3 e0 = float3(
        Triangles[triangle_pos + stride].v     - Triangles[triangle_pos].v,
        Triangles[triangle_pos + stride + 1].v - Triangles[triangle_pos + 1].v,
        Triangles[triangle_pos + stride + 2].v - Triangles[triangle_pos + 2].v
    );

    float3 e1 = float3(
        Triangles[triangle_pos + stride * 2].v     - Triangles[triangle_pos].v,
        Triangles[triangle_pos + stride * 2 + 1].v - Triangles[triangle_pos + 1].v,
        Triangles[triangle_pos + stride * 2 + 2].v - Triangles[triangle_pos + 2].v
    );

    float3 q = cross(ray.d, e1);
    float a = dot(e0, q);
    
    // TODO: a is said to be uninitialized by the compiler. Why?
    if (a > -EPSILON && a < EPSILON)
    {
        return FLOAT32_INVALID;
    }

    float f = 1.f / a;
    float3 s = float3(
        ray.o.x - Triangles[triangle_pos].v,
        ray.o.y - Triangles[triangle_pos + 1].v,
        ray.o.z - Triangles[triangle_pos + 2].v
    );

    float u = f * dot(s, q);

    if (u < 0.f)
    {
        return FLOAT32_INVALID;
    }

    float3 r = cross(s, e0);
    float v = f * dot(ray.d, r);

    if (v < 0.f || u + v > 1.f)
    {
        return FLOAT32_INVALID;
    }

    float t = f * dot(e1, r);

    return t;
}

float intersect_aabb(in float3 bmin, in float3 bmax, in Ray ray)
{
    float tx1 = (bmin.x - ray.o.x) * ray.invd.x;
    float tx2 = (bmax.x - ray.o.x) * ray.invd.x;
    float tmin = min(tx1, tx2);
    float tmax = max(tx1, tx2);
    float ty1 = (bmin.y - ray.o.y) * ray.invd.y;
    float ty2 = (bmax.y - ray.o.y) * ray.invd.y;
    tmin = max(tmin, min(ty1, ty2));
    tmax = min(tmax, max(ty1, ty2));
    float tz1 = (bmin.z - ray.o.z) * ray.invd.z;
    float tz2 = (bmax.z - ray.o.z) * ray.invd.z;
    tmin = max(tmin, min(tz1, tz2));
    tmax = min(tmax, max(tz1, tz2));

    if (tmax >= tmin && tmin < ray.t && tmax > 0)
    {
        return tmin;
    }
    else
    {
        return FLOAT32_INVALID;
    }
}

float intersect_bvh(Ray ray, in uint stride)
{
    uint stack[64];
    uint node_idx = 0;
    uint stack_ptr = 0;
    uint triangle_size = stride * VERT_PER_TRIANGLE + VERT_PER_TRIANGLE;

    while (true)
    {
        if (BVHNodes[node_idx].tri_count > 0)    // leaf node if true
        {
            for (uint i = 0; i < BVHNodes[node_idx].tri_count; ++i)
            {
                uint triangle_pos =
                    compute_triangle_pos(i + BVHNodes[node_idx].left_first, stride);
                
                float t = ray_hit_triangle(ray, triangle_pos, stride);
                if (t < ray.t)
                {
                    ray.t = t;
                }
            }

            if (stack_ptr == 0)
            {
                break;
            } 
            else
            {
                node_idx = stack[--stack_ptr];
            }

            continue;
        }

        uint child1_idx = BVHNodes[node_idx].left_first;
        uint child2_idx = BVHNodes[node_idx].left_first + 1;

        float dist1 = intersect_aabb(
            BVHNodes[child1_idx].aabbmin,
            BVHNodes[child1_idx].aabbmax,
            ray
        );

        float dist2 = intersect_aabb(
            BVHNodes[child2_idx].aabbmin,
            BVHNodes[child2_idx].aabbmax,
            ray
        );

        if (dist1 > dist2)
        {
            swap_float(dist1, dist2);
            swap_uint(child1_idx, child2_idx);
        }

        if (dist1 == FLOAT32_INVALID)
        {
            if (stack_ptr == 0)
            {
                break;
            } 
            else
            {
                node_idx = stack[--stack_ptr];
            }
        } 
        else
        {
            node_idx = child1_idx;
            if (dist2 != FLOAT32_INVALID)
            {
                stack[stack_ptr++] = child2_idx;
            }
        }
    }

    return ray.t;
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(CS_Input IN)
{
    const uint idx_cutoff = RayCamera.resolution.x * RayCamera.resolution.y;
    const uint global_threadID = IN.DispatchThreadID.x * IN.DispatchThreadID.y;

    if (idx_cutoff < global_threadID)
    {
        return;
    }

    Ray ray = generate_ray(IN.DispatchThreadID.x, IN.DispatchThreadID.y);
    float t = intersect_bvh(ray, triangle_stride_struct.stride);

    if (t < FLOAT32_INVALID)
    {
        float s = 2.f;
        output_texture[IN.DispatchThreadID.xy] = float4(t, t, 0.f, 1.f);
    }
    else
    {
        output_texture[IN.DispatchThreadID.xy] = float4(0.7f, 0.f, 0.7f, 1.f);
    }
}