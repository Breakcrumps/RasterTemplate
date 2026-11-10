#ifndef UNIFORMS_H
#define UNIFORMS_H

#include "matrices.h"
#include <stdbool.h>

typedef enum shade_mode
{
    SHADE_DEBUG = 1,
    SHADE_DERIVATIVE = 2,
    SHADE_DEPTH_ONLY = 4,
    SHADE_SOLID = 8
} ShadeMode;

typedef struct uniforms
{
    Mat4x4 view, proj, vp;
    float near, far;
    Vec3 l_clip;
    float lod_bias, tex_scale;
    ShadeMode shade_mode;
    bool cull_backfaces;
} Uniforms;

#endif
