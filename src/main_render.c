#define _USE_MATH_DEFINES
#include <float.h>
#include <inttypes.h>
#include "mesh_utils.h"
#include "matrices.h"
#include "scenes.h"
#include "image_utils.h"
#include "uniforms.h"

int main(int argc, char *argv[])
{
    int w = 1024, h = 768;
    const char *m_path = "resources/Bunny.obj";
    Mesh model = load_obj(m_path);
    mesh_normalize(model);
    mesh_compute_normals(&model);
    
    int tw, th;
    float *tex = load_image_f32_rgb("resources/textures/porcelain.jpg", &tw, &th, 2.2f);

    float *color = calloc((size_t)w * h * 3, sizeof(float));
    if (!color)
        return -1;

    const size_t pixel_count = (size_t)w * h;
    uint8_t *pixels = calloc(pixel_count << 2, 1);
    if (!pixels)
    {
        free(color);
        return -1;
    }
    float *depth = malloc(pixel_count * sizeof(float));
    if (!depth)
    {
        free(color), free(pixels);
        return -1;
    }
    for (size_t i = 0; i < pixel_count; i++)
    {
        depth[i] = FLT_MAX;
    }

    SceneObj object = {
        .mesh = &model,
        .tex = tex,
        .tex_w = tw,
        .tex_h = th,
        .fallback_albedo = make_v3(.85f, .80f, .72f),
        .instance_count = 1,
        .instances = malloc(sizeof(Mat4x4)),
    };
    object.instances[0] = trs_m4x4((float)M_PI / 6.f, Y, make_v3(0.f, 0.f, 0.f), 1.f);
    Scene scene = { .obj_count = 1, .objects = &object };

    const float fov_y  = (float)M_PI / 3.f;
    const float aspect = (float)w / h;
    const float near = 0.1f, far = 100.f;

    Mat4x4 view = look_at_m4x4(
        make_v3(0.f, 0.f, 1.5f),
        make_v3(0.f, 0.f, 0.f),
        make_v3(0.f, 1.f, 0.f)
    );
    Mat3x3 view3 = {
        view.m[0][0], view.m[0][1], view.m[0][2],
        view.m[1][0], view.m[1][1], view.m[1][2],
        view.m[2][0], view.m[2][1], view.m[2][2],
    };
    Vec3 l_world = normalized3(make_v3(-.5f, .8f, .4f));

    Uniforms uni = {
        .proj = perspective_m4x4(fov_y, aspect, near, far),
        .view = view,
        .l_view = normalized3(mul_m3x3_v3(view3, l_world)),
        .near = near, .far = far,
        .cull_backfaces = true
    };

    uni.vp = mul4x4(uni.proj, uni.view);

    Vec4 *scratch_clip = malloc(model.vert_count * sizeof(Vec4));
    Vec3 *scratch_norm = malloc(model.vert_count * sizeof(Vec3));

    scene_draw(pixels, depth, w, h, &scene, &uni, scratch_clip, scratch_norm);

    for (size_t i = 0; i < pixel_count; i++)
    {
        color[i * 3] = pixels[i * 4] / 255.f;
        color[i * 3 + 1] = pixels[i * 4 + 1] / 255.f;
        color[i * 3 + 2] = pixels[i * 4 + 2] / 255.f;
    }

    int ok = save_image_f32_png_rgb(color, "result.png", w, h, 3, 2.2f);
    return 0;
}
