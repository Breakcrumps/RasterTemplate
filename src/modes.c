#include "modes.h"

#include "vectors.h"
#include "matrices.h"
#include "draw_tri.h"

void tri_mode(
    uint8_t *pixels,
    float *hue, float hue_speed,
    int fb_width, int fb_height,
    float dt, double now
)
{
    *hue = fmodf(*hue + hue_speed * dt, 1.f);
    float rgb[3];
    hue_to_rgb(*hue, rgb);
    uint8_t r = to_u8(rgb[0]), g = to_u8(rgb[1]), b = to_u8(rgb[2]);

    float center_x = fb_width / 2.f;
    float center_y = fb_height / 2.f;
    float radius = 150.f;
    float time_factor = now;

    Vec2 v1 = { center_x + radius * cosf(time_factor), center_y + radius * sinf(time_factor) };
    Vec2 v2 = { center_x + radius * cosf(time_factor + 2.f), center_y + radius * sinf(time_factor + 2.f) };
    Vec2 v3 = { center_x + radius * cosf(time_factor + 4.f), center_y + radius * sinf(time_factor + 4.f) };

    draw_tri(pixels, fb_width, fb_height, v1, v2, v3, r, g, b);
}

void task2_mode(uint8_t *pixels, int fb_width, int fb_height, double now)
{
    const float fov_y = (float)M_PI / 3.f;
    const float aspect = (float)fb_width / fb_height;

    const float near = .1f, far = 100.f;

    Mat4x4 p = perspective_m4x4(fov_y, aspect, near, far);
    Mat4x4 v = look_at_m4x4(
        make_v3(0.f, 0.f, 3.f),
        make_v3(0.f, 0.f, 0.f),
        make_v3(0.f, 1.f, 0.f)
    );
    Mat4x4 m = mul4x4(rot_m4x4(now, Y), rot_m4x4(now * .7f, X));
    Mat4x4 mvp = mul4x4(p, mul4x4(v, m));

    static Vec3 cube[8] = {
        { -.7f, -.7f, -.7f}, { .7f, -.7f, -.7f },
        { .7f, .7f, -.7f }, { -.7f, .7f, -.7f },
        { -.7f, -.7f, .7f }, { .7f, -.7f, .7f },
        { .7f, .7f, .7f }, { -.7f, .7f, .7f },
    };
    static int faces[12][3] = {
        { 0, 1, 2 }, { 0, 2, 3 },
        { 4, 5, 6 }, { 4, 6, 7 },
        { 0, 1, 5 }, { 0, 5, 4 },
        { 3, 2, 6 }, { 3, 6, 7 },
        { 0, 3, 7 }, { 0, 7, 4 },
        { 1, 2, 6 }, { 1, 6, 5 },
    };

    Vec4 clip[8];
    Vec2 screen[8];
    for (int i = 0; i < 8; i++)
    {
        clip[i] = mul_m4x4_v4(mvp, make_v4(cube[i].x, cube[i].y, cube[i].z, 1.f));
        if (clip[i].w <= near)
            return;
        float inv_w = 1.f / clip[i].w;
        screen[i].x = (clip[i].x * inv_w * .5f + .5f) * fb_width;
        screen[i].y = (clip[i].y * inv_w * .5f + .5f) * fb_height;
    }

    typedef struct { float z; int f; } FaceOrder;
    FaceOrder order[12];
    for (int i = 0; i < 12; i++)
    {
        float zs = 0.f;
        for (int k = 0; k < 3; k++)
        {
            int vi = faces[i][k];
            zs += clip[vi].z / clip[vi].w;
        }
        order[i].z = zs / 3.f;
        order[i].f = i;
    }
    for (int i = 0; i < 12; i++)
    {
        for (int j = i + 1; j < 12; j++)
        {
            if (order[j].z > order[i].z)
            {
                FaceOrder tmp = order[i];
                order[i] = order[j];
                order[j] = tmp;
            }
        }
    }

    float rgb[3];
    hue_to_rgb(fmodf(now * .15f, 1.f), rgb);
    uint8_t r = to_u8(rgb[0]), g = to_u8(rgb[1]), b = to_u8(rgb[2]);

    for (int k = 0; k < 12; k++)
    {
        int i = order[k].f;
        draw_tri(
            pixels, fb_width, fb_height,
            screen[faces[i][0]],
            screen[faces[i][1]],
            screen[faces[i][2]],
            r, g, b
        );
    }
}

void mode_3d_mesh(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    Mesh mesh, double now
)
{
    const float fov_y = (float)M_PI / 3.f;
    const float aspect = (float)fb_width / fb_height;

    const float near = .1f, far = 100.f;

    Mat4x4 p = perspective_m4x4(fov_y, aspect, near, far);
    Mat4x4 view = look_at_m4x4(
        make_v3(0.f, 0.f, 2.f),
        make_v3(0.f, 0.f, 0.f),
        make_v3(0.f, 1.f, 0.f)
    );
    Mat4x4 m = rot_m4x4(now, Y);
    Mat4x4 mvp = mul4x4(p, mul4x4(view, m));

    for (int i = 0; i < mesh.tri_count; i++)
    {
        Vec3 p0 = mesh.verts[mesh.indices[i * 3]];
        Vec3 p1 = mesh.verts[mesh.indices[i * 3 + 1]];
        Vec3 p2 = mesh.verts[mesh.indices[i * 3 + 2]];

        Vec4 c0 = mul_m4x4_v4(mvp, make_v4(p0.x, p0.y, p0.z, 1.f));
        Vec4 c1 = mul_m4x4_v4(mvp, make_v4(p1.x, p1.y, p1.z, 1.f));
        Vec4 c2 = mul_m4x4_v4(mvp, make_v4(p2.x, p2.y, p2.z, 1.f));

        if (c0.w <= near || c1.w <= near || c2.w <= near)
            continue;

        Vec2 s0 = { (c0.x / c0.w * .5f + .5f) * fb_width,
                    (c0.y / c0.w * .5f + .5f) * fb_height };
        Vec2 s1 = { (c1.x / c1.w * .5f + .5f) * fb_width,
                    (c1.y / c1.w * .5f + .5f) * fb_height };
        Vec2 s2 = { (c2.x / c2.w * .5f + .5f) * fb_width,
                    (c2.y / c2.w * .5f + .5f) * fb_height };

        int cull_backfaces = 1;
        draw_tri_depth(pixels, depth, fb_width, fb_height,
                       s0, s1, s2,
                       c0.z / c0.w, c1.z / c1.w, c2.z / c2.w,
                       255, 255, 255,
                       cull_backfaces
        );
    }

    depth_to_grayscale(pixels, depth, (size_t)fb_width * fb_height);
}

void mode_lambert(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    Mesh mesh, double now)
{
    const float fov_y = (float)M_PI / 3.f;
    const float aspect = (float)fb_width / fb_height;

    const float near = .1f, far = 100.f;

    Mat4x4 p = perspective_m4x4(fov_y, aspect, near, far);
    Mat4x4 v = look_at_m4x4(
        make_v3(0.f, 0.f, 2.f),
        make_v3(0.f, 0.f, 0.f),
        make_v3(0.f, 1.f, 0.f));
    Mat4x4 m = rot_m4x4(now, Y);
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

    Vec3 albedo = make_v3(.85f, .8f, .72f);

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

        if (c0.w <= near || c1.w <= near || c2.w <= near)
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

void mode_tex(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    Mesh mesh, double now,
    const float *tex, int tex_w, int tex_h)
{
    const float fov_y = (float)M_PI / 3.f;
    const float aspect = (float)fb_width / fb_height;

    const float near = .1f, far = 100.f;

    Mat4x4 p = perspective_m4x4(fov_y, aspect, near, far);
    Mat4x4 v = look_at_m4x4(
        make_v3(0.f, 0.f, 2.f),
        make_v3(0.f, 0.f, 0.f),
        make_v3(0.f, 1.f, 0.f));
    Mat4x4 m = rot_m4x4(now, Y);
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

    Vec3 albedo = make_v3(.85f, .8f, .72f);

    const float *tex_used = mesh.tcs ? tex : NULL;

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

        if (c0.w <= near || c1.w <= near || c2.w <= near)
            continue;

        Vec2 s0 = { (c0.x / c0.w * .5f + .5f) * fb_width,
                    (c0.y / c0.w * .5f + .5f) * fb_height };
        Vec2 s1 = { (c1.x / c1.w * .5f + .5f) * fb_width,
                    (c1.y / c1.w * .5f + .5f) * fb_height };
        Vec2 s2 = { (c2.x / c2.w * .5f + .5f) * fb_width,
                    (c2.y / c2.w * .5f + .5f) * fb_height };

        Vec3 n0 = normalized3(mul_m3x3_v3(nrm_mat, mesh.normals[i0]));
        Vec3 n1 = normalized3(mul_m3x3_v3(nrm_mat, mesh.normals[i1]));
        Vec3 n2 = normalized3(mul_m3x3_v3(nrm_mat, mesh.normals[i2]));

        Vec2 uv0, uv1, uv2;
        if (mesh.tcs)
        {
            uv0 = mesh.tcs[i0];
            uv1 = mesh.tcs[i1];
            uv2 = mesh.tcs[i2];
        }
        else
        {
            uv0 = uv1 = uv2 = make_v2(0.f, 0.f);
        }

        int cull_backfaces = 1;
        draw_tri_tex(
            pixels, depth, fb_width, fb_height,
            s0, s1, s2,
            n0, n1, n2,
            uv0, uv1, uv2,
            c0.z / c0.w, c1.z / c1.w, c2.z / c2.w,
            1.f / c0.w, 1.f / c1.w, 1.f / c2.w,
            albedo, l_view,
            tex_used, tex_w, tex_h,
            cull_backfaces
        );
    }
}
