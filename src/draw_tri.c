#include "draw_tri.h"

#include <math.h>
#include "vectors.h"
#include "misc.h"

void draw_tri(
    uint8_t *pixels, int width, int height,
    Vec2 v1, Vec2 v2, Vec2 v3,
    uint8_t r, uint8_t g, uint8_t b
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
    
    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            Vec2 p = { x + .5f, y + .5f };

            float w0 = edge(v2, v3, p);
            float w1 = edge(v3, v1, p);
            float w2 = edge(v1, v2, p);

            float u = w0 / area, v = w1 / area, w = w2 / area;

            if (u >= 0.f && v >= 0.f && w >= 0.f)
            {
                size_t p_idx = ((size_t)y * width + x) << 2;
                pixels[p_idx] = r;
                pixels[p_idx + 1] = g;
                pixels[p_idx + 2] = b;
                pixels[p_idx + 3] = 255;
            }
        }
    }
}

void draw_tri_depth(
    uint8_t *pixels, float *depth, int width, int height,
    Vec2 v1, Vec2 v2, Vec2 v3,
    float z1, float z2, float z3,
    uint8_t r, uint8_t g, uint8_t b,
    int cull_backfaces
)
{
    float area = edge(v1, v2, v3);
    if (fabsf(area) < 1e-6f)
        return;

    if (cull_backfaces && area > 0.0f)
        return;

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
    if (min_x > max_x || min_y > max_y)
        return;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            Vec2 p = { x + .5f, y + .5f };
            float w0 = edge(v2, v3, p);
            float w1 = edge(v3, v1, p);
            float w2 = edge(v1, v2, p);
            float u = w0 / area, v = w1 / area, w = w2 / area;

            if (u < 0.0f || v < 0.0f || w < 0.0f)
                continue;

            float z = u * z1 + v * z2 + w * z3;
            size_t idx = (size_t)y * width + x;
            if (z >= depth[idx])
                continue;

            depth[idx] = z;
            pixels[idx << 2] = r;
            pixels[(idx << 2) + 1] = g;
            pixels[(idx << 2) + 2] = b;
            pixels[(idx << 2) + 3] = 255;
        }
    }
}

void draw_tri_lambert(
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
    float ambient = .15f;
    float diffuse = .85f;

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

void draw_tri_tex(
    uint8_t *pixels, float *depth, int width, int height,
    Vec2 s0, Vec2 s1, Vec2 s2,
    Vec3 n0, Vec3 n1, Vec3 n2,
    Vec2 uv0, Vec2 uv1, Vec2 uv2,
    float z0, float z1, float z2,
    float invw0, float invw1, float invw2,
    Vec3 albedo, Vec3 L_dir,
    const float *tex, int tex_w, int tex_h,
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
    float ambient = .15f;
    float diffuse = .85f;

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

            Vec2 uv = {
                (bu * uv0.x * invw0 + bv * uv1.x * invw1 + bw * uv2.x * invw2) * inv_t,
                (bu * uv0.y * invw0 + bv * uv1.y * invw1 + bw * uv2.y * invw2) * inv_t,
            };

            Vec3 albedo_used = tex ? tex_sample_bilinear(tex, tex_w, tex_h, uv.x, uv.y) : albedo;

            float ndotl = dot3(N, L_dir);
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
