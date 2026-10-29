#ifndef SCENES_H
#define SCENES_H

#include <inttypes.h>
#include "mesh_utils.h"
#include "matrices.h"
#include "texture.h"

struct uniforms;

typedef struct scene_obj
{
  const Mesh *mesh;
  Mat4x4 *instances;
  Texture tex;
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
    static uint8_t lut[256];
    static int ready = 0;
    if (!ready)
    {
        for (int i = 0; i < 256; i++)
        {
            lut[i] = powf(i / 255.f, 1.f / 2.2f) * 255.f + .5f;
        }
        ready = 1;
    }
    for (size_t i = 0; i < fb_size; i++)
    {
        pixels[i << 2] = lut[pixels[i << 2]];
        pixels[(i << 2) + 1] = lut[pixels[(i << 2) + 1]];
        pixels[(i << 2) + 2] = lut[pixels[(i << 2) + 2]];
    }
}

#endif
