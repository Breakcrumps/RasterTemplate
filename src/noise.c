#include "noise.h"
#include <math.h>

static inline unsigned int hash_uint(unsigned int x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = ((x >> 16) ^ x) * 0x45d9f3bu;
    x = (x >> 16) ^ x;
    return x;
}

static inline float fade5(float t) {
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}
static inline float lerp_f(float a, float b, float t) {
    return a + (b - a) * t;
}

static float grad2(int ix, int iy, float fx, float fy, unsigned int seed) {
    unsigned int h = hash_uint(
        (unsigned int)ix * 374761393u
        + (unsigned int)iy * 668265263u
        + seed * 2654435761u
    );
    switch (h & 7u)
    {
        case 0:
            return fx + fy;
        case 1:
            return -fx + fy;
        case 2:
            return fx - fy;
        case 3:
            return -fx - fy;
        case 4:
            return fx;
        case 5:
            return -fx;
        case 6:
            return fy;
        default:
            return -fy;
    }
}

float perlin2d(float x, float y, unsigned int seed)
{
    int ix = floorf(x);
    int iy = floorf(y);
    float fx = x - ix, fy = y - iy;
    float u = fade5(fx), v = fade5(fy);
    float n00 = grad2(ix, iy, fx, fy, seed);
    float n10 = grad2(ix + 1, iy, fx - 1, fy, seed);
    float n01 = grad2(ix, iy + 1, fx, fy - 1, seed);
    float n11 = grad2(ix + 1, iy + 1, fx - 1, fy - 1, seed);
    return lerp_f(lerp_f(n00, n10, u), lerp_f(n01, n11, u), v);
}

float fbm2d(float x, float y, int octaves, float lacunarity, float gain, unsigned int seed)
{
    float sum = 0.f, amp = 1.f, freq = 1.f;
    for (int i = 0; i < octaves; i++)
    {
        sum += amp * perlin2d(x * freq, y * freq, seed + (unsigned int)i * 1013u);
        freq *= lacunarity;
        amp *= gain;
    }
    return sum;
}
