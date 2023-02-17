#pragma once
#include "aabb.hpp"
#include "plane.hpp"
#include "frustum.hpp"

#include <immintrin.h>

namespace moonlight
{

// Modified version of OBB/Plane test from "Real-time Collision Detection", Christer Ericson
inline bool plane_intersects_aabb(const Plane& plane, const AABB& aabb)
{
    float d = dot(plane.normal, plane.point);

    Vector3 c = (aabb.bmax + aabb.bmin) * 0.5f;
    Vector3 e = aabb.bmax - c;

    float r = e.x * abs(plane.normal.x) + e.y * abs(plane.normal.y) + e.z * abs(plane.normal.z);
    float s = dot(plane.normal, c) - d;

    return abs(s) <= r;
}

inline bool aabb_in_positive_halfspace_of_plane(const Plane& plane, const AABB& aabb)
{
    float d = dot(plane.normal, plane.point);

    Vector3 c = (aabb.bmax + aabb.bmin) * 0.5f;
    Vector3 e = aabb.bmax - c;

    float r = e.x * abs(plane.normal.x) + e.y * abs(plane.normal.y) + e.z * abs(plane.normal.z);
    float s = dot(plane.normal, c) - d;
    
    return s > r;
}

inline bool aabb_intersects_positive_halfspace_of_plane(const Plane& plane, const AABB& aabb)
{
    float d = dot(plane.normal, plane.point);

    Vector3 c = (aabb.bmax + aabb.bmin) * 0.5f;
    Vector3 e = aabb.bmax - c;

    float r = e.x * abs(plane.normal.x) + e.y * abs(plane.normal.y) + e.z * abs(plane.normal.z);
    float s = dot(plane.normal, c) - d;

    return s > r || abs(s) <= r;
}

inline bool frustum_contains_aabb(const Plane* planes, const AABB& aabb, float* d)
{
    Vector3 c = (aabb.bmax + aabb.bmin) * 0.5f;
    Vector3 e = aabb.bmax - c;

    for (int i = 0; i < 6; ++i)
    {
        float r = e.x * abs(planes[i].normal.x) + e.y * abs(planes[i].normal.y) + e.z * abs(planes[i].normal.z);
        float s = dot(planes[i].normal, c) - d[i];

        if (s > r)
        {
            return false;
        }
    }

    return true;
}

struct alignas(32) AABB256
{
    float bmin_x[8];
    float bmax_x[8];
    float bmin_y[8];
    float bmax_y[8];
    float bmin_z[8];
    float bmax_z[8];
};

struct alignas(32) PlaneSIMD
{
    float nx[8];
    float ny[8];
    float nz[8];
};

struct alignas(32) FrustumSIMD
{
    PlaneSIMD normals[6];
};

// Perform 8 AABB intersection tests against @frustum.
// See description for avx2 version
inline uint8_t frustum_contains_aabb_sse4(
    const FrustumSIMD* frustum,
    const FrustumSIMD* abs_frustum,
    const AABB256& aabb,
    float* d)
{
    uint8_t output_mask = 0xFF;
    for (int k = 0; k < 2; ++k)
    {
        __m128 r0 = _mm_set_ps1(0.5f);
        __m128 e0 = _mm_load_ps(&aabb.bmax_x[k * 4]);
        __m128 e1 = _mm_load_ps(&aabb.bmax_y[k * 4]);
        __m128 e2 = _mm_load_ps(&aabb.bmax_z[k * 4]);
        __m128 r1 = _mm_load_ps(&aabb.bmin_x[k * 4]);
        __m128 cx = _mm_add_ps(e0, r1);

        r1 = _mm_load_ps(&aabb.bmin_y[k * 4]);
        __m128 cy = _mm_add_ps(e1, r1);
        r1 = _mm_load_ps(&aabb.bmin_z[k * 4]);
        __m128 cz = _mm_add_ps(e2, r1);

        cx = _mm_mul_ps(cx, r0);
        cy = _mm_mul_ps(cy, r0);
        cz = _mm_mul_ps(cz, r0);

        // Vector3 e = aabb.bmax - c;
        e0 = _mm_sub_ps(e0, cx); // ex
        e1 = _mm_sub_ps(e1, cy); // ey
        e2 = _mm_sub_ps(e2, cz); // ez

        for (int i = 0; i < 6; ++i)
        {
            // float s = dot(planes[i].normal, c) - d[i];
            // float s = p[i].normal.x * cx + p[i].normal.y * cy + p[i].normal.z * cz
            __m128 s = _mm_load_ps(frustum->normals[i].nx);
            __m128 r2 = _mm_load_ps(frustum->normals[i].ny);
            r0 = _mm_load_ps(frustum->normals[i].nz);
            r1 = _mm_load_ps(&d[i * 8]);
            s = _mm_fmsub_ps(s, cx, r1);
            s = _mm_fmadd_ps(r2, cy, s);
            s = _mm_fmadd_ps(r0, cz, s);

            // float r = e.x * abs(planes[i].normal.x) + e.y * abs(planes[i].normal.y) + e.z * abs(planes[i].normal.z);
            r0 = _mm_load_ps(abs_frustum->normals[i].nx);
            r1 = _mm_load_ps(abs_frustum->normals[i].ny);
            r2 = _mm_load_ps(abs_frustum->normals[i].nz);
            r0 = _mm_mul_ps(e0, r0);
            r0 = _mm_fmadd_ps(e1, r1, r0);
            r0 = _mm_fmadd_ps(e2, r2, r0);

            // if (s > r) return false;
            r0 = _mm_cmp_ps(s, r0, 0x12); // 0x12 stands for _CMP_LE_OQ (LESS THAN OR EQUAL TO)
            uint8_t sign_mask = _mm_movemask_ps(r0);
            uint8_t lo_mask = output_mask >> (k * 4);
            lo_mask = lo_mask & sign_mask;
            output_mask = (lo_mask << (k * 4)) | (output_mask & (0xF << ((1-k) * 4)));
        }
    }

    return output_mask;
}

// Perform 8 AABB intersection tests against @frustum.
// The result is returned as a uint8_t bitmask, where there
// positive bits describe an AABB that is inside the frustum and vice versa.
inline uint8_t frustum_contains_aabb_avx2(
    const FrustumSIMD* frustum,
    const FrustumSIMD* abs_frustum,
    const AABB256& aabb,
    float* d)
{
    uint8_t output_mask = 0xFF;
    //Vector3 c = (aabb.bmax + aabb.bmin) * 0.5f;
    __m256 r0 = _mm256_set1_ps(0.5f);
    __m256 e0 = _mm256_load_ps(aabb.bmax_x);
    __m256 e1 = _mm256_load_ps(aabb.bmax_y);
    __m256 e2 = _mm256_load_ps(aabb.bmax_z);
    __m256 r1 = _mm256_load_ps(aabb.bmin_x);
    __m256 cx = _mm256_add_ps(e0, r1);
    
    r1 = _mm256_load_ps(aabb.bmin_y);
    __m256 cy = _mm256_add_ps(e1, r1);
    r1 = _mm256_load_ps(aabb.bmin_z);
    __m256 cz = _mm256_add_ps(e2, r1);

    cx = _mm256_mul_ps(cx, r0);
    cy = _mm256_mul_ps(cy, r0);
    cz = _mm256_mul_ps(cz, r0);

    // Vector3 e = aabb.bmax - c;
    e0 = _mm256_sub_ps(e0, cx); // ex
    e1 = _mm256_sub_ps(e1, cy); // ey
    e2 = _mm256_sub_ps(e2, cz); // ez

    for (int i = 0; i < 6; ++i)
    {
        // float s = dot(planes[i].normal, c) - d[i];
        // float s = p[i].normal.x * cx + p[i].normal.y * cy + p[i].normal.z * cz
        __m256 s = _mm256_load_ps(frustum->normals[i].nx);
        __m256 r2 = _mm256_load_ps(frustum->normals[i].ny);
        r0 = _mm256_load_ps(frustum->normals[i].nz);
        r1 = _mm256_load_ps(&d[i * 8]);
        s = _mm256_fmsub_ps(s, cx, r1);
        s = _mm256_fmadd_ps(r2, cy, s);
        s = _mm256_fmadd_ps(r0, cz, s);

        // float r = e.x * abs(planes[i].normal.x) + e.y * abs(planes[i].normal.y) + e.z * abs(planes[i].normal.z);
        r0 = _mm256_load_ps(abs_frustum->normals[i].nx);
        r1 = _mm256_load_ps(abs_frustum->normals[i].ny);
        r2 = _mm256_load_ps(abs_frustum->normals[i].nz);
        r0 = _mm256_mul_ps(e0, r0);
        r0 = _mm256_fmadd_ps(e1, r1, r0);
        r0 = _mm256_fmadd_ps(e2, r2, r0);

        // if (s > r) return false;
        r0 = _mm256_cmp_ps(s, r0, 0x12); // 0x12 stands for _CMP_LE_OQ (LESS THAN OR EQUAL TO)
        uint8_t sign_mask = _mm256_movemask_ps(r0);
        output_mask = output_mask & sign_mask;
    }

    return output_mask;
}

}