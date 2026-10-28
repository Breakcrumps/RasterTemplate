#ifndef MODES_H
#define MODES_H

#define _USE_MATH_DEFINES
#include <inttypes.h>
#include <math.h>
#include "misc.h"
#include "mesh_utils.h"

static void grad_mode(uint8_t *pixels, size_t pixel_count, float *hue, float hue_speed, float dt)
{
    *hue = fmodf(*hue + hue_speed * dt, 1.f);

    float rgb[3];
    hue_to_rgb(*hue, rgb);
    const uint8_t r = to_u8(rgb[0]), g = to_u8(rgb[1]), b = to_u8(rgb[2]);

    for (size_t i = 0; i < pixel_count; i++)
    {
        pixels[4 * i + 0] = r;
        pixels[4 * i + 1] = g;
        pixels[4 * i + 2] = b;
        pixels[4 * i + 3] = 255;
    }
}

void tri_mode(
    uint8_t *pixels,
    float *hue, float hue_speed,
    int fb_width, int fb_height,
    float dt, double now
);

void task2_mode(uint8_t *pixels, int fb_width, int fb_height, double now);

void mode_3d_mesh(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    Mesh mesh, double now
);

void mode_lambert(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    Mesh mesh, double now
);

void mode_tex(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    Mesh mesh, double now,
    const float *tex, int tex_w, int tex_h
);

#endif
