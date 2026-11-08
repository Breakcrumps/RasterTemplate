#include "texture.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "image_utils.h"

static int mip_count(int w, int h)
{
    int n = 1;
    while (w > 1 || h > 1)
    {
        w >>= 1, h >>= 1;
        n++;
    }
    return n;
}

static float *downsample_2x2(const float *in, int in_w, int in_h, int *dw, int *dh)
{
    int new_w = in_w > 1 ? in_w >> 1 : 1;
    int new_h = in_h > 1 ? in_h >> 1 : 1;
    float *dst = malloc((size_t)new_w * new_h * 3 * sizeof(float));
    if (!dst)
        return NULL;

    for (int y = 0; y < new_h; y++)
    {
        for (int x = 0; x < new_w; x++)
        {
            int x0 = x << 1, x1 = (x << 1) + 1 < in_w ? (x << 1) + 1 : in_w - 1;
            int y0 = y << 1, y1 = (y << 1) + 1 < in_h ? (y << 1) + 1 : in_h - 1;
            const float *c00 = &in[(y0 * in_w + x0) * 3];
            const float *c10 = &in[(y0 * in_w + x1) * 3];
            const float *c01 = &in[(y1 * in_w + x0) * 3];
            const float *c11 = &in[(y1 * in_w + x1) * 3];
            float *o = &dst[(y * new_w + x) * 3];
            for (int ch = 0; ch < 3; ch++)
            {
                o[ch] = .25f * (c00[ch] + c10[ch] + c01[ch] + c11[ch]);
            }
        }
    }
    *dw = new_w; *dh = new_h;
    return dst;
}

Texture texture_from_pixels(const float *pixels, int w, int h)
{
    Texture t = {0};
    if (!pixels || w <= 0 || h <= 0)
        return t;

    int n = mip_count(w, h);
    t.levels = malloc(n * sizeof(float *));
    t.ws = malloc(n * sizeof(int));
    t.hs = malloc(n * sizeof(int));
    t.count = n;

    t.levels[0] = pixels;
    t.ws[0] = w;
    t.hs[0] = h;

    for (int l = 1; l < n; l++)
    {
        int dw, dh;
        t.levels[l] = downsample_2x2(t.levels[l - 1], t.ws[l - 1], t.hs[l - 1], &dw, &dh);
        t.ws[l] = dw;
        t.hs[l] = dh;
        if (!t.levels[l])
        {
            t.count = l;
            break;
        }
    }
    return t;
}

Texture texture_load(const char *path, float gamma)
{
    int w = 0, h = 0;
    float *pixels = load_image_f32_rgb(path, &w, &h, gamma);
    return texture_from_pixels(pixels, w, h);
}

void texture_free(Texture *t)
{
    if (!t->levels)
        return;
    for (int l = 0; l < t->count; l++)
        free(t->levels[l]);
    free(t->levels);
    free(t->ws);
    free(t->hs);
    *t = (Texture){0};
}
