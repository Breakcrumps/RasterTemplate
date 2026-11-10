#ifndef TEXTURE_H
#define TEXTURE_H

struct vec3;

typedef struct texture
{
    float **levels;
    int *ws, *hs;
    int count;
} Texture;

Texture texture_load(const char *path, float gamma);
Texture texture_from_pixels(float *pixels, int w, int h);
void texture_free(Texture *t);

struct vec3 texture_sample_bilinear_level(const Texture *t, int level, float u, float v);
struct vec3 texture_sample_trilinear(const Texture *t, float u, float v, float lod);

#endif
