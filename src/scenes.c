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

        const Texture *tex = mesh->tcs ? &obj->tex : NULL;

        for (int t = 0; t < mesh->tri_count; t++)
        {
            int i1 = mesh->indices[3 * t];
            int i2 = mesh->indices[3 * t + 1];
            int i3 = mesh->indices[3 * t + 2];

            Vec4 c[3] = { scratch_clip[i1], scratch_clip[i2], scratch_clip[i3] };
            if (c[0].w <= uni->near && c[1].w <= uni->near && c[2].w <= uni->near)
                continue;
            Vec3 ns[3] = { scratch_norm[i1], scratch_norm[i2], scratch_norm[i3] };
            Vec2 uvs[3];
            if (mesh->tcs)
            {
                uvs[0] = mesh->tcs[i1];
                uvs[1] = mesh->tcs[i2];
                uvs[2] = mesh->tcs[i3];
            }

            Vec4 cout[4];
            Vec3 nout[4];
            Vec2 uvout[4];
            int n = clip_near(c, ns, uvs, uni->near, cout, nout, uvout);
            if (n < 3)
                continue;

            for (int k = 1; k + 1 < n; k++)
            {
                Vertex v1 = make_vertex(cout[0], nout[0], uvout[0], fb_width, fb_height);
                Vertex v2 = make_vertex(cout[k], nout[k], uvout[k],  fb_width, fb_height);
                Vertex v3 = make_vertex(cout[k+1], nout[k+1], uvout[k+1], fb_width, fb_height);

                draw_tri_tex(
                    pixels, depth, fb_width, fb_height,
                    v1, v2, v3, obj->fallback_albedo,
                    uni, tex
                );
            }
        }
    }
}
