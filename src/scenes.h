#ifndef SCENES_H
#define SCENES_H

#include <inttypes.h>
#include "mesh_utils.h"
#include "matrices.h"

struct uniforms;

typedef struct scene_obj
{
  const Mesh *mesh;
  const float *tex;
  Mat4x4 *instances;
  int tex_w, tex_h;
  Vec3 fallback_albedo;
  int instance_count;
} SceneObj;

typedef struct scene
{
  SceneObj *objects;
  int obj_count;
} Scene;

void draw_obj(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    const SceneObj *obj, const struct uniforms *uni,
    Vec4 *scratch_clip, Vec3 *scratch_norm
);

static void scene_draw(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    const Scene *scene, const struct uniforms *uni,
    Vec4 *scratch_clip, Vec3 *scratch_norm)
{
    for (int i = 0; i < scene->obj_count; i++)
    {
        draw_obj(
            pixels, depth, fb_width, fb_height,
            &scene->objects[i], uni,
            scratch_clip, scratch_norm
        );
    }
}

static inline void gamma_correct(uint8_t *pixels, size_t fb_size)
{
    for (size_t i = 0; i < fb_size; i++)
    {
        pixels[i << 2] = powf(pixels[i << 2] / 255.f, 1.f / 2.2f) * 255.f;
        pixels[(i << 2) + 1] = powf(pixels[(i << 2) + 1] / 255.f, 1.f / 2.2f) * 255.f;
        pixels[(i << 2) + 2] = powf(pixels[(i << 2) + 2] / 255.f, 1.f / 2.2f) * 255.f;
    }
}

#endif
