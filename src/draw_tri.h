#ifndef DRAW_H
#define DRAW_H

#include <inttypes.h>

struct vec2;
struct vec3;

void draw_tri(
    uint8_t *pixels, int width, int height,
    struct vec2 v1, struct vec2 v2, struct vec2 v3,
    uint8_t r, uint8_t g, uint8_t b
);

void draw_tri_depth(
    uint8_t *pixels, float *depth, int width, int height,
    struct vec2 v1, struct vec2 v2, struct vec2 v3,
    float z1, float z2, float z3,
    uint8_t r, uint8_t g, uint8_t b,
    int cull_backfaces
);

void draw_tri_lambert(
    uint8_t *pixels, float *depth, int width, int height,
    struct vec2 s0, struct vec2 s1, struct vec2 s2,
    struct vec3 n0, struct vec3 n1, struct vec3 n2,
    float z0, float z1, float z2,
    float invw0, float invw1, float invw2,
    struct vec3 albedo, struct vec3 L_dir,
    int cull_backfaces
);

void draw_tri_tex(
    uint8_t *pixels, float *depth, int width, int height,
    struct vec2 s0, struct vec2 s1, struct vec2 s2,
    struct vec3 n0, struct vec3 n1, struct vec3 n2,
    struct vec2 uv0, struct vec2 uv1, struct vec2 uv2,
    float z0, float z1, float z2,
    float invw0, float invw1, float invw2,
    struct vec3 albedo, struct vec3 L_dir,
    const float *tex, int tex_w, int tex_h,
    int cull_backfaces
);

#endif
