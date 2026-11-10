#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef enum upscale_mode { UPSCALE_NEAREST, UPSCALE_BILINEAR, UPSCALE_WHOLE, UPSCALE_NONE } UpscaleMode;

static void nearest(
    uint8_t *out, int out_w, int out_h,
    const uint8_t *in, int in_w, int in_h
)
{
    for (int y = 0; y < out_h; y++)
    {
        int sy = y * in_h / out_h;
        for (int x = 0; x < out_w; x++)
        {
            int sx = x * in_w / out_w;
            out[(y * out_w + x) << 2] = in[(sy * in_w + sx) << 2];
            out[((y * out_w + x) << 2) + 1] = in[((sy * in_w + sx) << 2) + 1];
            out[((y * out_w + x) << 2) + 2] = in[((sy * in_w + sx) << 2) + 2];
            out[((y * out_w + x) << 2) + 3] = 255;
        }
    }
}

void bilinear(
    uint8_t *out, int out_w, int out_h,
    const uint8_t *in, int in_w, int in_h
);

static void whole(
    uint8_t *out, int out_w,
    const uint8_t *in, int in_w, int in_h
)
{
    int k = out_w / in_w;
    for (int y = 0; y < in_h; y++)
    {
        for (int x = 0; x < in_w; x++)
        {
            uint32_t pix;
            memcpy(&pix, &in[(y * in_w + x) << 2], 4);
            for (int dy = 0; dy < k; dy++)
            {
                for (int dx = 0; dx < k; dx++)
                {
                    memcpy(&out[((y * k + dy) * out_w + x * k + dx) << 2], &pix, 4);
                }
            }

        }
    }
}

#endif
