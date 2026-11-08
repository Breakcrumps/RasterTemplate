#include "interpolation.h"

#include <math.h>

void bilinear(
  uint8_t *out, int out_w, int out_h,
  const uint8_t *in, int in_w, int in_h
)
{
    float x_scale = (float)in_w / out_w;
    float y_scale = (float)in_h / out_h;
    for (int y = 0; y < out_h; y++)
    {
        float fy = (y + .5f) * y_scale - .5f;
        int y0 = floorf(fy), y1 = y0 + 1;
        if (y0 < 0)
            y0 = 0;
        if (y1 < 0)
            y1 = 0;
        if (y1 >= in_h)
            y1 = in_h - 1;
        if (y0 >= in_h)
            y0 = in_h - 1;
        float ty = fy - y0;

        for (int x = 0; x < out_w; x++)
        {
            float fx = (x + .5f) * x_scale - .5f;
            int x0 = floorf(fx), x1 = x0 + 1;
            if (x0 < 0)
                x0 = 0;
            if (x1 < 0)
                x1 = 0;
            if (x1 >= in_w)
                x1 = in_w - 1;
            if (x0 >= in_w)
                x0 = in_w - 1;
            float tx = fx - x0;

            const uint8_t *c00 = &in[(y0 * in_w + x0) << 2];
            const uint8_t *c10 = &in[(y0 * in_w + x1) << 2];
            const uint8_t *c01 = &in[(y1 * in_w + x0) << 2];
            const uint8_t *c11 = &in[(y1 * in_w + x1) << 2];

            float w00 = (1.f - tx) * (1.f - ty), w10 = tx * (1.f - ty);
            float w01 = (1.f - tx) * ty, w11 = tx * ty;

            uint8_t *o = &out[(y * out_w + x) << 2];
            for (int ch = 0; ch < 3; ch++)
            {
                float v = w00 * c00[ch] + w10 * c10[ch] + w01 * c01[ch] + w11 * c11[ch];
                o[ch] = v + .5f;
            }
            o[3] = 255;
        }
    }
}
