#pragma once

namespace moonlight
{

struct IntersectionParams
{
    float t = 1e30;     // Ray's t scale
    float u, v;         // Reserved for now (relevant for textured surfaces)
};

}