#ifndef UNIFORMS_H
#define UNIFORMS

#include "matrices.h"

typedef struct uniforms
{
  Mat4x4 view, proj;
  float near, far;
  Vec3 light_pos;
  bool cull_backfaces;
} Uniforms;

#endif
