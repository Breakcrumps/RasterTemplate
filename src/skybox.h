#ifndef SKYBOX_H
#define SKYBOX_H

#include "texture.h"
#include <inttypes.h>

struct vec3;
struct uniforms;

typedef struct cubemap
{
    Texture faces[6]; // -+x, +-y, +-z (but not in geometry mode)
} Cubemap;

void draw_skybox_ray(
    uint8_t *pixels, const float *depth, int width, int height,
    const Cubemap *cm, const struct uniforms *uni
);
void draw_skybox_geom(
    uint8_t *pixels, float *depth, int width, int height,
    const Cubemap *cm, const struct uniforms *uni, struct vec3 camera_pos
);
Cubemap cubemap_load(const char *paths[6]);
void cubemap_free(Cubemap *cm);

#endif
