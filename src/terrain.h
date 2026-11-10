#ifndef TERRAIN_H
#define TERRAIN_H

#include <inttypes.h>
#include "vectors.h"

struct uniforms;
struct texture;

typedef struct terrain_vert
{
    Vec4 clip;
    Vec3 n_view;
    Vec2 uv;
    float blend;
} TerrainVert;

typedef struct terrain
{
    float *heightmap;
    int hm_w, hm_h;
    float world_size;
    float height_scale;

    int grid_n;
    float step;
    TerrainVert *cache;
} Terrain;

void terrain_init(
    Terrain *t, int grid_n, float step,
    float height_scale,
    unsigned int seed, int octaves
);
void terrain_free(Terrain *t);
void terrain_save_heightmap_png(const Terrain *t, const char *path);

float terrain_main_loop_sample(const Terrain *t, float wx, float wz);
Vec3 terrain_sample_normal(const Terrain *t, float wx, float wz);

void terrain_draw(
    uint8_t *pixels, float *depth, int width, int height,
    Terrain *t, Vec3 camera_pos,
    const struct uniforms *uni,
    const struct texture *grass, const struct texture *rock
);

#endif
