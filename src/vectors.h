#ifndef VECTORS_H
#define VECTORS_H

// Минимальный набор векторных типов, нужный утилитам шаблона.
// Остальную математику (операции над векторами, матрицы) вы пишете сами.

#include <inttypes.h>

typedef struct vec2
{
    union
    {
        struct { float x, y; };
        float items[2];
    };
} Vec2;
typedef struct vec3
{
    union
    {
        struct { float x, y, z; };
        float items[3];
    };
} Vec3;
typedef struct color
{
    union
    {
        struct { uint8_t r, g, b; };
        uint8_t items[3];
    };
} Color;
typedef struct vec4
{
    union
    {
        struct { float x, y, z, w; };
        float items[4];
    };
} Vec4;

static inline Vec2 make_v2(float x, float y)
{
    return (Vec2){ x, y };
}
static inline Vec3 make_v3(float x, float y, float z)
{
    return (Vec3){ x, y, z };
}
static inline Color make_color(uint8_t r, uint8_t g, uint8_t b)
{
    return (Color){ r, g, b };
}
static inline Vec4 make_v4(float x, float y, float z, float w)
{
    return (Vec4){ x, y, z, w };
}

static inline Vec3 sub3(Vec3 a, Vec3 b)
{
    return (Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline float edge(Vec2 a, Vec2 b, Vec2 c)
{
    return (c.y - a.y) * (b.x - a.x) - (c.x - a.x) * (b.y - a.y);
}

static inline float dot3(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 cross3(Vec3 a, Vec3 b)
{
    return (Vec3){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline Vec3 normalized3(Vec3 v)
{
    float lensq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lensq <= 0)
        return v;
    float term = 1.f / sqrtf(lensq);
    return (Vec3){ v.x * term, v.y * term, v.z * term };
}

#endif
