#include "texture.h"

#include <stdlib.h>
#include <math.h>
#include "image_utils.h"
#include "vectors.h"
#include "misc.h"

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

Texture texture_from_pixels(float *pixels, int w, int h)
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

Vec3 texture_sample_bilinear_level(const Texture *t, int level, float u, float v)
{
    if (!t || !t->levels || level < 0 || level >= t->count)
        return make_v3(1.f, 1.f, 1.f);
    return tex_sample_bilinear(t->levels[level], t->ws[level], t->hs[level], u, v);
}

Vec3 texture_sample_trilinear(const Texture *t, float u, float v, float lod)
{
    if (!t || !t->levels)
        return make_v3(1.f, 1.f, 1.f);

    if (lod < 0.f)
        lod = 0.f;
    if (lod > t->count - 1)
        lod = t->count - 1;

    int l0 = floorf(lod);
    float f  = lod - l0;
    int l1 = l0 + 1 < t->count ? l0 + 1 : l0;

    Vec3 c0 = texture_sample_bilinear_level(t, l0, u, v);
    if (f < 1e-4f || l0 == l1)
        return c0;

    Vec3 c1 = texture_sample_bilinear_level(t, l1, u, v);
    return make_v3(
        c0.x * (1.f - f) + c1.x * f,
        c0.y * (1.f - f) + c1.y * f,
        c0.z * (1.f - f) + c1.z * f
    );
}
