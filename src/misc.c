#include "misc.h"

#include <float.h>
#include "vectors.h"

Vec3 tex_sample_bilinear(const float *tex, int w, int h, float u, float v)
{
    if (!tex || w <= 0 || h <= 0)
        return make_v3(1.f, 1.f, 1.f);

    u -= floorf(u);
    v -= floorf(v);

    float x = u * w - .5f;
    float y = v * h - .5f;

    int x0 = floorf(x);
    int y0 = floorf(y);
    float fx = x - x0;
    float fy = y - y0;

    int x1 = x0 + 1;
    int y1 = y0 + 1;

    x0 = ((x0 % w) + w) % w;
    x1 = ((x1 % w) + w) % w;
    y0 = ((y0 % h) + h) % h;
    y1 = ((y1 % h) + h) % h;

    const float *c00 = &tex[3 * (y0 * w + x0)];
    const float *c10 = &tex[3 * (y0 * w + x1)];
    const float *c01 = &tex[3 * (y1 * w + x0)];
    const float *c11 = &tex[3 * (y1 * w + x1)];

    float w00 = (1.f - fx) * (1.f - fy);
    float w10 = fx * (1.f - fy);
    float w01 = (1.f - fx) * fy;
    float w11 = fx * fy;

    return make_v3(
        w00*c00[0] + w10*c10[0] + w01*c01[0] + w11*c11[0],
        w00*c00[1] + w10*c10[1] + w01*c01[1] + w11*c11[1],
        w00*c00[2] + w10*c10[2] + w01*c01[2] + w11*c11[2]
    );
}

void depth_to_grayscale(uint8_t *pixels, const float *depth, size_t pixel_count)
{
    float zmin = FLT_MAX, zmax = FLT_MIN;
    for (size_t i = 0; i < pixel_count; i++)
    {
        if (depth[i] == FLT_MAX)
            continue;
        if (depth[i] < zmin)
            zmin = depth[i];
        if (depth[i] > zmax)
            zmax = depth[i];
    }
    if (zmax < zmin)
        return;
    float inv_range = (zmax - zmin > 1e-8f) ? (1.f / (zmax - zmin)) : 1.f;

    for (size_t i = 0; i < pixel_count; i++)
    {
        if (depth[i] == FLT_MAX)
            continue;
        float t = (depth[i] - zmin) * inv_range;
        uint8_t g = (1.f - t) * 255.f;
        pixels[i << 2] = g;
        pixels[(i << 2) + 1] = g;
        pixels[(i << 2) + 2] = g;
        pixels[(i << 2) + 3] = 255;
    }
}
