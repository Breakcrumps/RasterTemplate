#ifndef TEXTURE_H
#define TEXTURE_H

// typedef struct texture
// {
//     float *pixels;
//     int w, h;
// } Texture;

typedef struct texture
{
    float **levels;
    int *ws, *hs;
    int count;
} Texture;

Texture texture_load(const char *path, float gamma);
Texture texture_from_pixels(const float *pixels, int w, int h);
void texture_free(Texture *t);

#endif
