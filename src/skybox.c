#include "skybox.h"
#include "vectors.h"
#include "uniforms.h"
#include "misc.h"
#include "vertex.h"
#include "draw_tri.h"
#include <float.h>

Cubemap cubemap_load(const char *paths[6])
{
    Cubemap cm;
    for (int i = 0; i < 6; i++)
    {
        cm.faces[i] = texture_load(paths[i], 2.2f); 
    }
    return cm;
}

void cubemap_free(Cubemap *cm)
{
    for (int i = 0; i < 6; i++)
    {
        texture_free(&cm->faces[i]);
    }
}

static Vec3 cubemap_sample(const Cubemap *cm, Vec3 dir)
{
    float absX = fabsf(dir.x);
    float absY = fabsf(dir.y);
    float absZ = fabsf(dir.z);

    int face_index = 0;
    float u = 0.f, v = 0.f;
    float max_axis;

    if (absX >= absY && absX >= absZ)
    {
        max_axis = absX;
        if (dir.x > 0)
        {
            face_index = 0;
            u = dir.z; v = dir.y;
        }
        else
        {
            face_index = 1;
            u = -dir.z; v = dir.y;
        }
    }
    else if (absY >= absX && absY >= absZ)
    {
        max_axis = absY;
        if (dir.y > 0)
        {
            face_index = 2;
            u = dir.x; v = dir.z;
        }
        else
        {
            face_index = 3;
            u = dir.x; v = -dir.z;
        }
    }
    else
    {
        max_axis = absZ;
        if (dir.z > 0)
        {
            face_index = 4;
            u = -dir.x; v = dir.y;
        }
        else
        {
            face_index = 5;
            u = dir.x; v = dir.y;
        }
    }

    u = .5f * (u / max_axis + 1.f);
    v = .5f * (v / max_axis + 1.f);

    float w = cm->faces[face_index].ws[0]; 
    float h = cm->faces[face_index].hs[0];

    u = (u * (w - 1.f) + .5f) / w;
    v = (v * (h - 1.f) + .5f) / h;

    return texture_sample_bilinear_level(&cm->faces[face_index], 0, u, v);
}

void draw_skybox_ray(
    uint8_t *pixels, const float *depth, int width, int height,
    const Cubemap *cm, const Uniforms *uni
)
{
    Mat4x4 inv_proj;
    if (!inv4x4(&inv_proj, uni->proj))
        return;

    Mat4x4 view_rot = uni->view;
    view_rot.m[0][3] = 0.f;
    view_rot.m[1][3] = 0.f;
    view_rot.m[2][3] = 0.f;
    
    Mat4x4 inv_view_rot;
    if (!inv4x4(&inv_view_rot, view_rot))
        return;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            size_t idx = (size_t)y * width + x;

            if (depth[idx] < FLT_MAX)
                continue;

            float nx = ((x + .5f) / width) * 2.f - 1.f;
            float ny = ((y + .5f) / height) * 2.f - 1.f; 

            Vec4 clip_pos = make_v4(nx, ny, -1.f, 1.f);

            Vec4 view_pos = mul_m4x4_v4(inv_proj, clip_pos);
            view_pos.z = -1.f; 
            view_pos.w = 0.f;

            Vec4 world_dir4 = mul_m4x4_v4(inv_view_rot, view_pos);
            Vec3 dir = normalized3(make_v3(world_dir4.x, world_dir4.y, world_dir4.z));

            Vec3 color = cubemap_sample(cm, dir);

            pixels[idx << 2] = to_u8(color.x);
            pixels[(idx << 2) + 1] = to_u8(color.y);
            pixels[(idx << 2) + 2] = to_u8(color.z);
            pixels[(idx << 2) + 3] = 255;
        }
    }
}

static void draw_tri_skybox(
    uint8_t *pixels, float *depth, int width, int height,
    Vertex v1, Vertex v2, Vertex v3,
    Vec3 albedo,
    const Texture *tex
)
{
    float area = edge(v1.scr, v2.scr, v3.scr);
    if (fabsf(area) < 1e-6f)
        return;

    int min_x = fminf(v1.scr.x, fminf(v2.scr.x, v3.scr.x));
    int max_x = fmaxf(v1.scr.x, fmaxf(v2.scr.x, v3.scr.x));
    int min_y = fminf(v1.scr.y, fminf(v2.scr.y, v3.scr.y));
    int max_y = fmaxf(v1.scr.y, fmaxf(v2.scr.y, v3.scr.y));
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

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            size_t idx = (size_t)y * width + x;
            if (depth[idx] != FLT_MAX)
                continue;
            
            Vec2 p = { x + .5f, y + .5f };

            float bu = edge(v2.scr, v3.scr, p) * inv_area;
            float bv = edge(v3.scr, v1.scr, p) * inv_area;
            float bw = edge(v1.scr, v2.scr, p) * inv_area;
            if (bu < 0.f || bv < 0.f || bw < 0.f)
                continue;

            float t = bu * v1.invw + bv * v2.invw + bw * v3.invw;
            float inv_t = 1.f / t;
            Vec3 n = {
                (bu * v1.n.x * v1.invw + bv * v2.n.x * v2.invw + bw * v3.n.x * v3.invw) * inv_t,
                (bu * v1.n.y * v1.invw + bv * v2.n.y * v2.invw + bw * v3.n.y * v3.invw) * inv_t,
                (bu * v1.n.z * v1.invw + bv * v2.n.z * v2.invw + bw * v3.n.z * v3.invw) * inv_t,
            };
            n = normalized3(n);

            Vec2 uv = {
                (bu * v1.uv.x * v1.invw + bv * v2.uv.x * v2.invw + bw * v3.uv.x * v3.invw) * inv_t,
                (bu * v1.uv.y * v1.invw + bv * v2.uv.y * v2.invw + bw * v3.uv.y * v3.invw) * inv_t,
            };

            Vec3 albedo_used;
            if (tex && tex->levels)
            {
                albedo_used = tex_sample_bilinear(tex->levels[0], tex->ws[0], tex->hs[0], uv.x, uv.y);
            }
            else
            {
                albedo_used = albedo;
            }

            pixels[idx << 2] = to_u8(albedo_used.x);
            pixels[(idx << 2) + 1] = to_u8(albedo_used.y);
            pixels[(idx << 2) + 2] = to_u8(albedo_used.z);
            pixels[(idx << 2) + 3] = 255;
        }
    }
}

static void draw_skybox_face(
    uint8_t *pixels, float *depth, int w, int h,
    Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3,
    const Texture *tex, const Uniforms *uni
)
{
    Vec4 c[4] = {
        mul_m4x4_v4(uni->vp, make_v4(p0.x, p0.y, p0.z, 1.f)),
        mul_m4x4_v4(uni->vp, make_v4(p1.x, p1.y, p1.z, 1.f)),
        mul_m4x4_v4(uni->vp, make_v4(p2.x, p2.y, p2.z, 1.f)),
        mul_m4x4_v4(uni->vp, make_v4(p3.x, p3.y, p3.z, 1.f))
    };

    float inset_u = .5f / tex->ws[0]; // ширина половины пикселя
    float inset_v = .5f / tex->hs[0];

    Vec2 uvs[4] = {
        { 0.f + inset_u, 0.f + inset_v }, // Минус швы
        { 1.f - inset_u, 0.f + inset_v },
        { 1.f - inset_u, 1.f - inset_v },
        { 0.f + inset_u, 1.f - inset_v }
    };
    Vec3 dummy_norm = uni->l_clip;

    int tri_indices[2][3] = { { 0, 1, 2 }, { 0, 2, 3 } };

    for (int t = 0; t < 2; t++)
    {
        int i0 = tri_indices[t][0];
        int i1 = tri_indices[t][1];
        int i2 = tri_indices[t][2];

        Vec4 c_tri[3] = { c[i0], c[i1], c[i2] };
        Vec3 n_tri[3] = { dummy_norm, dummy_norm, dummy_norm };
        Vec2 uv_tri[3] = { uvs[i0], uvs[i1], uvs[i2] };

        if (c_tri[0].w <= uni->near && c_tri[1].w <= uni->near && c_tri[2].w <= uni->near)
            continue;

        Vec4 cout[4]; Vec3 nout[4]; Vec2 uvout[4];
        int n = clip_near(c_tri, n_tri, uv_tri, uni->near, cout, nout, uvout);
        if (n < 3)
            continue;

        for (int k = 1; k + 1 < n; k++)
        {
            Vertex v1 = make_vertex(cout[0], nout[0], uvout[0], w, h);
            Vertex v2 = make_vertex(cout[k], nout[k], uvout[k], w, h);
            Vertex v3 = make_vertex(cout[k + 1], nout[k + 1], uvout[k + 1], w, h);

            draw_tri_skybox(
                pixels, depth, w, h,
                v1, v2, v3, make_v3(1.f, 1.f, 1.f),
                tex
            );
        }
    }
}

void draw_skybox_geom(
    uint8_t *pixels, float *depth, int width, int height,
    const Cubemap *cm, const Uniforms *uni, Vec3 camera_pos
)
{
    float r = 10.f;

    float x0 = camera_pos.x - r, x1 = camera_pos.x + r;
    float y0 = camera_pos.y - r, y1 = camera_pos.y + r;
    float z0 = camera_pos.z - r, z1 = camera_pos.z + r;

    Vec3 v000 = { x0, y0, z0 }, v100 = { x1, y0, z0 }, v110 = { x1, y1, z0 }, v010 = { x0, y1, z0 };
    Vec3 v001 = { x0, y0, z1 }, v101 = { x1, y0, z1 }, v111 = { x1, y1, z1 }, v011 = { x0, y1, z1 };

    draw_skybox_face(pixels, depth, width, height, v101, v100, v110, v111, &cm->faces[0], uni);
    draw_skybox_face(pixels, depth, width, height, v000, v001, v011, v010, &cm->faces[1], uni);
    draw_skybox_face(pixels, depth, width, height, v011, v111, v110, v010, &cm->faces[2], uni);
    draw_skybox_face(pixels, depth, width, height, v001, v101, v100, v000, &cm->faces[3], uni);
    draw_skybox_face(pixels, depth, width, height, v100, v000, v010, v110, &cm->faces[4], uni);
    draw_skybox_face(pixels, depth, width, height, v001, v101, v111, v011, &cm->faces[5], uni);
}
