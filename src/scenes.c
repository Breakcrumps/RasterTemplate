#include "scenes.h"

#include "draw_tri.h"
#include "uniforms.h"
#include "vertex.h"
#include "texture.h"

void draw_obj(
    uint8_t *pixels, float *depth, int fb_width, int fb_height,
    const SceneObj *obj, const Uniforms *uni,
    Vec4 *scratch_clip, Vec3 *scratch_norm
)
{
    const Mesh *mesh = obj->mesh;

    for (int i = 0; i < obj->instance_count; i++)
    {
        Mat4x4 mv = mul4x4(uni->view, obj->instances[i]);
        Mat4x4 mvp = mul4x4(uni->vp, obj->instances[i]);

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

        Texture tex = {
            .pixels = mesh->tcs ? obj->tex.pixels : NULL,
            .w = obj->tex.w, .h = obj->tex.h
        };

        for (int t = 0; t < mesh->tri_count; t++)
        {
            int i1 = mesh->indices[3 * t];
            int i2 = mesh->indices[3 * t + 1];
            int i3 = mesh->indices[3 * t + 2];

            Vec4 c1 = scratch_clip[i1];
            Vec4 c2 = scratch_clip[i2];
            Vec4 c3 = scratch_clip[i3];
            if (c1.w <= uni->near || c2.w <= uni->near || c3.w <= uni->near)
                continue;

            float invw1 = 1.f / c1.w;
            float invw2 = 1.f / c2.w;
            float invw3 = 1.f / c3.w;

            Vertex v1 = {
                .invw = invw1, .n = scratch_norm[i1],
                .scr = (Vec2){
                    (c1.x * invw1 * .5f + .5f) * fb_width,
                    (c1.y * invw1 * .5f + .5f) * fb_height
                },
                .depth = c1.z * invw1
            };
            Vertex v2 = {
                .invw = invw2, .n = scratch_norm[i2],
                .scr = (Vec2){
                    (c2.x * invw2 * .5f + .5f) * fb_width,
                    (c2.y * invw2 * .5f + .5f) * fb_height
                },
                .depth = c2.z * invw2
            };
            Vertex v3 = {
                .invw = invw3, .n = scratch_norm[i3],
                .scr = (Vec2){
                    (c3.x * invw3 * .5f + .5f) * fb_width,
                    (c3.y * invw3 * .5f + .5f) * fb_height
                },
                .depth = c3.z * invw3
            };

            if (mesh->tcs)
            {
                v1.uv = mesh->tcs[i1];
                v2.uv = mesh->tcs[i2];
                v3.uv = mesh->tcs[i3];
            }

            draw_tri_tex(
                pixels, depth, fb_width, fb_height,
                v1, v2, v3,
                obj->fallback_albedo, uni->l_clip,
                tex, uni->cull_backfaces
            );
        }
    }
}
