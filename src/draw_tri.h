#ifndef DRAW_H
#define DRAW_H

#include <inttypes.h>

struct vec2;
struct vec3;
struct color;
struct vertex;
struct texture;

void draw_tri(
    uint8_t *pixels, int width, int height,
    struct vec2 v0, struct vec2 v1, struct vec2 v2,
    struct color c1, struct color c2, struct color c3
);

void draw_tri_tex(
    uint8_t *pixels, float *depth, int width, int height,
    struct vertex v1, struct vertex v2, struct vertex v3,
    struct vec3 albedo, struct vec3 L_clip,
    struct texture tex, int cull_backfaces
);

#endif
