#define _USE_MATH_DEFINES
#include <float.h>
#include <inttypes.h>
#include <string.h>
#include "mesh_utils.h"
#include "matrices.h"
#include "scenes.h"
#include "image_utils.h"
#include "uniforms.h"
#include "draw_tri.h"
#include "misc.h"
#include "texture.h"

static inline void draw_tri_lambert(
    uint8_t *pixels, float *depth, int width, int height,
    Vec2 s0, Vec2 s1, Vec2 s2,
    Vec3 n0, Vec3 n1, Vec3 n2,
    float z0, float z1, float z2,
    float invw0, float invw1, float invw2,
    Vec3 albedo, Vec3 L_dir,
    int cull_backfaces
)
{
    float area = edge(s0, s1, s2);
    if (fabsf(area) < 1e-6f)
        return;
    if (cull_backfaces && area > 0.f)
        return;

    int min_x = fminf(s0.x, fminf(s1.x, s2.x));
    int max_x = fmaxf(s0.x, fmaxf(s1.x, s2.x));
    int min_y = fminf(s0.y, fminf(s1.y, s2.y));
    int max_y = fmaxf(s0.y, fmaxf(s1.y, s2.y));
    if (min_x < 0)
        min_x = 0;
    if (min_y < 0)
        min_y = 0;
    if (max_x >= width)
        max_x = width - 1;
    if (max_y >= height)
        max_y = height - 1;
    if (min_x > max_x || min_y > max_y)
        return;

    float inv_area = 1.f / area;
    float ambient = .05f;
    float diffuse = .95f;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            Vec2 p = { x + .5f, y + .5f };

            float bu = edge(s1, s2, p) * inv_area;
            float bv = edge(s2, s0, p) * inv_area;
            float bw = edge(s0, s1, p) * inv_area;
            if (bu < 0.f || bv < 0.f || bw < 0.f)
                continue;

            float z = bu * z0 + bv * z1 + bw * z2;
            size_t idx = (size_t)y * width + x;
            if (z >= depth[idx])
                continue;

            float t = bu * invw0 + bv * invw1 + bw * invw2;
            float inv_t = 1.f / t;
            Vec3 N = {
                (bu * n0.x * invw0 + bv * n1.x * invw1 + bw * n2.x * invw2) * inv_t,
                (bu * n0.y * invw0 + bv * n1.y * invw1 + bw * n2.y * invw2) * inv_t,
                (bu * n0.z * invw0 + bv * n1.z * invw1 + bw * n2.z * invw2) * inv_t,
            };
            N = normalized3(N);

            float ndotl = dot3(N, L_dir);
            if (ndotl < 0.f)
                ndotl = 0.f;
            float shade = ambient + diffuse * ndotl;

            depth[idx] = z;
            pixels[idx << 2] = to_u8(albedo.x * shade);
            pixels[(idx << 2) + 1] = to_u8(albedo.y * shade);
            pixels[(idx << 2) + 2] = to_u8(albedo.z * shade);
            pixels[(idx << 2) + 3] = 255;
        }
    }
}

static void draw_lambert(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    Mesh mesh
)
{
    float fov_y = (float)M_PI / 3.f;
    float aspect = (float)fb_width / fb_height;

    Mat4x4 p = perspective_m4x4(fov_y, aspect, .1f, 100.f);
    Mat4x4 v = look_at_m4x4(
        make_v3(0.f, 0.f, 2.f),
        make_v3(0.f, 0.f, 0.f),
        make_v3(0.f, 1.f, 0.f));
    Mat4x4 m = (Mat4x4){
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    Mat4x4 mv = mul4x4(v, m);
    Mat4x4 mvp = mul4x4(p, mv);

    Mat3x3 mv3 = {
        mv.m[0][0], mv.m[0][1], mv.m[0][2],
        mv.m[1][0], mv.m[1][1], mv.m[1][2],
        mv.m[2][0], mv.m[2][1], mv.m[2][2],
    };
    Mat3x3 mv3_inv;
    Mat3x3 nrm_mat;
    if (inv3x3(&mv3_inv, mv3))
        nrm_mat = transposed3x3(mv3_inv);
    else
        nrm_mat = mv3;

    Vec3 l_world = normalized3(make_v3(.5f, .8f, .4f));

    Mat3x3 view3 = {
        v.m[0][0], v.m[0][1], v.m[0][2],
        v.m[1][0], v.m[1][1], v.m[1][2],
        v.m[2][0], v.m[2][1], v.m[2][2],
    };
    Vec3 l_view = normalized3(mul_m3x3_v3(view3, l_world));

    Vec3 albedo = make_v3(0.85f, 0.8f, 0.72f);

    for (int i = 0; i < mesh.tri_count; i++)
    {
        int i0 = mesh.indices[i * 3];
        int i1 = mesh.indices[i * 3 + 1];
        int i2 = mesh.indices[i * 3 + 2];

        Vec3 p0 = mesh.verts[i0];
        Vec3 p1 = mesh.verts[i1];
        Vec3 p2 = mesh.verts[i2];

        Vec4 c0 = mul_m4x4_v4(mvp, make_v4(p0.x, p0.y, p0.z, 1.f));
        Vec4 c1 = mul_m4x4_v4(mvp, make_v4(p1.x, p1.y, p1.z, 1.f));
        Vec4 c2 = mul_m4x4_v4(mvp, make_v4(p2.x, p2.y, p2.z, 1.f));

        if (c0.w <= 1e-6f || c1.w <= 1e-6f || c2.w <= 1e-6f)
            continue;

        Vec2 s0 = { (c0.x / c0.w * 0.5f + 0.5f) * fb_width,
                    (c0.y / c0.w * 0.5f + 0.5f) * fb_height };
        Vec2 s1 = { (c1.x / c1.w * 0.5f + 0.5f) * fb_width,
                    (c1.y / c1.w * 0.5f + 0.5f) * fb_height };
        Vec2 s2 = { (c2.x / c2.w * 0.5f + 0.5f) * fb_width,
                    (c2.y / c2.w * 0.5f + 0.5f) * fb_height };

        Vec3 n0 = normalized3(mul_m3x3_v3(nrm_mat, mesh.normals[i0]));
        Vec3 n1 = normalized3(mul_m3x3_v3(nrm_mat, mesh.normals[i1]));
        Vec3 n2 = normalized3(mul_m3x3_v3(nrm_mat, mesh.normals[i2]));

        int cull_backfaces = 1;
        draw_tri_lambert(
            pixels, depth, fb_width, fb_height,
            s0, s1, s2,
            n0, n1, n2,
            c0.z / c0.w, c1.z / c1.w, c2.z / c2.w,
            1.f / c0.w, 1.f / c1.w, 1.f / c2.w,
            albedo, l_view,
            cull_backfaces
        );
    }
}

int main(int argc, char *argv[])
{
    int w = 1024, h = 768;
    const char *m_path = "resources/Bunny.obj";
    Mesh model = load_obj(m_path);
    mesh_normalize(model);
    mesh_compute_normals(&model);
    
    Texture tex;
    tex.pixels = load_image_f32_rgb("resources/textures/porcelain.jpg", &tex.w, &tex.h, 2.2f);

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
        .l_clip = normalized3(mul_m3x3_v3(view3, l_world)),
        .near = near, .far = far,
        .cull_backfaces = true
    };

    uni.vp = mul4x4(uni.proj, uni.view);

    Vec4 *scratch_clip = malloc(model.vert_count * sizeof(Vec4));
    Vec3 *scratch_norm = malloc(model.vert_count * sizeof(Vec3));

    scene_draw(pixels, depth, w, h, &scene, &uni, scratch_clip, scratch_norm);
    for (size_t i = 0; i < pixel_count; i++)
    {
        color[i * 3] = pixels[i << 2] / 255.f;
        color[i * 3 + 1] = pixels[(i << 2) + 1] / 255.f;
        color[i * 3 + 2] = pixels[(i << 2) + 2] / 255.f;
    }
    save_image_f32_png_rgb(color, "6.png", w, h, 3, 2.2f);

    depth_to_grayscale(pixels, depth, pixel_count);
    for (size_t i = 0; i < pixel_count; i++)
    {
        color[i * 3] = pixels[i << 2] / 255.f;
        color[i * 3 + 1] = pixels[(i << 2) + 1] / 255.f;
        color[i * 3 + 2] = pixels[(i << 2) + 2] / 255.f;
    }
    save_image_f32_png_rgb(color, "3.png", w, h, 3, 2.2f);

    memset(pixels, 0, pixel_count << 2);
    for (size_t i = 0; i < pixel_count; i++)
    {
        depth[i] = FLT_MAX;
    }
    draw_lambert(pixels, depth, w, h, model);
    for (size_t i = 0; i < pixel_count; i++)
    {
        color[i * 3] = pixels[i << 2] / 255.f;
        color[i * 3 + 1] = pixels[(i << 2) + 1] / 255.f;
        color[i * 3 + 2] = pixels[(i << 2) + 2] / 255.f;
    }
    save_image_f32_png_rgb(color, "4.png", w, h, 3, 2.2f);

    memset(pixels, 0, pixel_count << 2);
    Vec2 v0 = { w * .5f, h * .1f };
    Vec2 v1 = { w * .1f, h * .9f };
    Vec2 v2 = { w * .9f, h * .9f };
    draw_tri(pixels, w, h, v0, v1, v2, (Color){ 255, 0, 0 }, (Color){ 0, 255, 0 }, (Color){ 0, 0, 255 });
    for (size_t i = 0; i < pixel_count; i++)
    {
        color[i * 3] = pixels[i << 2] / 255.f;
        color[i * 3 + 1] = pixels[(i << 2) + 1] / 255.f;
        color[i * 3 + 2] = pixels[(i << 2) + 2] / 255.f;
    }
    save_image_f32_png_rgb(color, "1.png", w, h, 3, 2.2f);

    free(pixels);
    free(color);
    free(depth);
    free(scratch_clip);
    free(scratch_norm);
    free(object.instances);
    free(tex.pixels);
    free_mesh(model);
    return 0;
}
