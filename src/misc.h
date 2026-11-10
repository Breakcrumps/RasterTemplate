#ifndef MISC_H
#define MISC_H

#include <math.h>
#include <inttypes.h>
#include "vectors.h"

static inline float clamp01(float x)
{
    return x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
}

static inline uint8_t to_u8(float x)
{
    return clamp01(x) * 255.f + .5f;
}

Vec3 tex_sample_bilinear(const float *tex, int w, int h, float u, float v);

void depth_to_grayscale(uint8_t *pixels, const float *depth, size_t pixel_count);

static inline float linear_depth_from_z_ndc(float z_ndc, float near, float far)
{
    return 2.f * near * far / ((far + near) - z_ndc * (far - near));
}

static inline float lod_by_depth(
    float z_ndc, float near, float far,
    float tex_scale, float lod_bias
)
{
    float d = linear_depth_from_z_ndc(z_ndc, near, far);
    float lod = log2f(d / tex_scale) + lod_bias;
    return lod < 0.f ? 0.f : lod;
}

static inline Vec3 lod_debug_color(float lod)
{
    static const Vec3 palette[8] = {
        {1.0f, 0.0f, 0.0f},
        {1.0f, 0.5f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 1.0f},
        {0.0f, 0.5f, 1.0f},
        {0.5f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f}
    };
    int lvl = floorf(lod + 0.5f);
    if (lvl < 0)
        lvl = 0;
    return palette[lvl & 7];
}

#endif
