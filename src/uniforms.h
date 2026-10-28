#ifndef UNIFORMS_H
#define UNIFORMS_H

#include "matrices.h"
#include <stdbool.h>

typedef struct uniforms
{
  Mat4x4 view, proj, vp;
  float near, far;
  Vec3 l_view;
  bool cull_backfaces;
} Uniforms;

#endif
