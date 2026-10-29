#ifndef MISC_H
#define MISC_H

#include <math.h>
#include <inttypes.h>

struct vec3;

static inline float clamp01(float x)
{
    return x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
}

static inline uint8_t to_u8(float x)
{
    return clamp01(x) * 255.f + .5f;
}

struct vec3 tex_sample_bilinear(const float *tex, int w, int h, float u, float v);

void depth_to_grayscale(uint8_t *pixels, const float *depth, size_t pixel_count);

#endif
