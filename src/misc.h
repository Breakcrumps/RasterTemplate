#ifndef MISC_H
#define MISC_H

#include <stdio.h>
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

static inline uint8_t to_srgb8(float x)
{
    static float lut[1024];
    static int ready = 0;
    if (!ready)
    {
        for (int i = 0; i < 1024; i++)
            lut[i] = powf((float)i / 1023.f, 1.f / 2.2f);
        ready = 1;
    }
    if (x <= 0.f)
        return 0;
    if (x >= 1.f)
        return 255;
    return lut[(int)(x * 1023.f + .5f)] * 255.f + .5f;
}

static inline void hue_to_rgb(float hue, float rgb[3])
{
    float h = hue * 6.f;
    rgb[0] = clamp01(fabsf(h - 3.f) - 1.f);
    rgb[1] = clamp01(2.f - fabsf(h - 2.f));
    rgb[2] = clamp01(2.f - fabsf(h - 4.f));
}

struct vec3 tex_sample_bilinear(const float *tex, int w, int h, float u, float v);

void depth_to_grayscale(uint8_t *pixels, const float *depth, size_t pixel_count);

#endif
