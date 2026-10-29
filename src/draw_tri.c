#include "draw_tri.h"

#include <math.h>
#include "vectors.h"
#include "texture.h"
#include "vertex.h"
#include "misc.h"

void draw_tri(
    uint8_t *pixels, int width, int height,
    Vec2 v1, Vec2 v2, Vec2 v3,
    Color c1, Color c2, Color c3
)
{
    int min_x = fminf(v1.x, fminf(v2.x, v3.x));
    int max_x = fmaxf(v1.x, fmaxf(v2.x, v3.x));
    int min_y = fminf(v1.y, fminf(v2.y, v3.y));
    int max_y = fmaxf(v1.y, fmaxf(v2.y, v3.y));

    if (min_x < 0)
        min_x = 0;
    if (min_y < 0)
        min_y = 0;
    if (max_x >= width)
        max_x = width - 1;
    if (max_y >= height)
        max_y = height - 1;

    float area = edge(v1, v2, v3);
    if (fabs(area) < 1e-4f)
        return;
    float inv_area = 1.f / area;
    
    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            Vec2 p = { x + .5f, y + .5f };

            float w0 = edge(v2, v3, p);
            float w1 = edge(v3, v1, p);
            float w2 = edge(v1, v2, p);

            float u = w0 * inv_area, v = w1 * inv_area, w = w2 * inv_area;

            if (u >= 0.f && v >= 0.f && w >= 0.f)
            {
                size_t p_idx = ((size_t)y * width + x) << 2;
                pixels[p_idx] = u * c1.r + v * c2.r + w * c3.r;
                pixels[p_idx + 1] = u * c1.g + v * c2.g + w * c3.g;
                pixels[p_idx + 2] = u * c1.b + v * c2.b + w * c3.b;
                pixels[p_idx + 3] = 255;
            }
        }
    }
}

void draw_tri_tex(
    uint8_t *pixels, float *depth, int width, int height,
    Vertex v1, Vertex v2, Vertex v3,
    Vec3 albedo, Vec3 L_clip,
    Texture tex, int cull_backfaces
)
{
    float area = edge(v1.scr, v2.scr, v3.scr);
    if (fabsf(area) < 1e-6f)
        return;
    if (cull_backfaces && area < 0.f)
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
    float ambient = .05f;
    float diffuse = .95f;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            Vec2 p = { x + .5f, y + .5f };

            float bu = edge(v2.scr, v3.scr, p) * inv_area;
            float bv = edge(v3.scr, v1.scr, p) * inv_area;
            float bw = edge(v1.scr, v2.scr, p) * inv_area;
            if (bu < 0.f || bv < 0.f || bw < 0.f)
                continue;

            float z = bu * v1.depth + bv * v2.depth + bw * v3.depth;
            size_t idx = (size_t)y * width + x;
            if (z >= depth[idx])
                continue;

            float t = bu * v1.invw + bv * v2.invw + bw * v3.invw;
            float inv_t = 1.f / t;
            Vec3 N = {
                (bu * v1.n.x * v1.invw + bv * v2.n.x * v2.invw + bw * v3.n.x * v3.invw) * inv_t,
                (bu * v1.n.y * v1.invw + bv * v2.n.y * v2.invw + bw * v3.n.y * v3.invw) * inv_t,
                (bu * v1.n.z * v1.invw + bv * v2.n.z * v2.invw + bw * v3.n.z * v3.invw) * inv_t,
            };
            N = normalized3(N);

            Vec2 uv = {
                (bu * v1.uv.x * v1.invw + bv * v2.uv.x * v2.invw + bw * v3.uv.x * v3.invw) * inv_t,
                (bu * v1.uv.y * v1.invw + bv * v2.uv.y * v2.invw + bw * v3.uv.y * v3.invw) * inv_t,
            };

            Vec3 albedo_used = tex.pixels ? tex_sample_bilinear(tex.pixels, tex.w, tex.h, uv.x, uv.y) : albedo;

            float ndotl = dot3(N, L_clip);
            if (ndotl < 0.f)
                ndotl = 0.f;
            float shade = ambient + diffuse * ndotl;

            depth[idx] = z;
            pixels[idx << 2] = to_u8(albedo_used.x * shade);
            pixels[(idx << 2) + 1] = to_u8(albedo_used.y * shade);
            pixels[(idx << 2) + 2] = to_u8(albedo_used.z * shade);
            pixels[(idx << 2) + 3] = 255;
        }
    }
}
