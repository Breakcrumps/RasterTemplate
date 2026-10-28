#include "scenes.h"

#include "draw_tri.h"
#include "uniforms.h"

void draw_obj(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    const SceneObj *obj, const Uniforms *uni,
    Vec4 *scratch_clip, Vec3 *scratch_norm
)
{
    const Mesh *mesh = obj->mesh;

    for (int i = 0; i < obj->instance_count; i++)
    {
        Mat4x4 m = obj->instances[i];
        Mat4x4 mv = mult4x4(uni->view, m);
        Mat4x4 mvp = mult4x4(uni->proj, mv);

        Mat3x3 mv3 = {
            mv.m[0][0], mv.m[0][1], mv.m[0][2],
            mv.m[1][0], mv.m[1][1], mv.m[1][2],
            mv.m[2][0], mv.m[2][1], mv.m[2][2],
        };
        Mat3x3 mv3_inv, nrm_mat;
        if (inv3x3(&mv3_inv, mv3))
            nrm_mat = transposed3x3(mv3_inv);
        else
            nrm_mat = mv3;

        for (int v = 0; v < mesh->vert_count; v++)
        {
            Vec3 p = mesh->verts[v];
            scratch_clip[v] = mul_m4x4_v4(mvp, make_v4(p.x, p.y, p.z, 1.f));
            scratch_norm[v] = !!mesh->normals
                ? normalized3(mul_m3x3_v3(nrm_mat, mesh->normals[v]))
                : make_v3(0.f, 0.f, 1.f);
        }

        for (int t = 0; t < mesh->tri_count; t++)
        {
            int i0 = mesh->indices[3 * t];
            int i1 = mesh->indices[3 * t + 1];
            int i2 = mesh->indices[3 * t + 2];

            Vec4 c0 = scratch_clip[i0];
            Vec4 c1 = scratch_clip[i1];
            Vec4 c2 = scratch_clip[i2];
            if (c0.w <= uni->near || c1.w <= uni->near || c2.w <= uni->near)
                continue;

            Vec2 s0 = { (c0.x / c0.w * .5f + .5f) * fb_width,
                        (c0.y / c0.w * .5f + .5f) * fb_height };
            Vec2 s1 = { (c1.x / c1.w * .5f + .5f) * fb_width,
                        (c1.y / c1.w * .5f + .5f) * fb_height };
            Vec2 s2 = { (c2.x / c2.w * .5f + .5f) * fb_width,
                        (c2.y / c2.w * .5f + .5f) * fb_height };

            Vec2 uv0, uv1, uv2;
            if (!!mesh->tcs) {
                uv0 = mesh->tcs[i0];
                uv1 = mesh->tcs[i1];
                uv2 = mesh->tcs[i2];
            } else {
                uv0 = uv1 = uv2 = make_v2(0.f, 0.f);
            }

            draw_tri_tex(
                pixels, depth, fb_width, fb_height,
                s0, s1, s2,
                scratch_norm[i0], scratch_norm[i1], scratch_norm[i2],
                uv0, uv1, uv2,
                c0.z / c0.w, c1.z / c1.w, c2.z / c2.w,
                1.f / c0.w, 1.f / c1.w, 1.f / c2.w,
                obj->fallback_albedo, uni->light_pos,
                obj->tex, obj->tex_w, obj->tex_h,
                uni->cull_backfaces
            );
        }
    }
}
