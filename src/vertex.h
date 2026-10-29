#ifndef VERTEX_H
#define VERTEX_H

#include "vectors.h"

typedef struct vertex
{
  Vec2 scr, uv;
  Vec3 n;
  float depth, invw;
} Vertex;

#endif
