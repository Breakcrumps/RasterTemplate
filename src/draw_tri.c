#include "draw_tri.h"

#include <math.h>
#include "vectors.h"
#include "texture.h"
#include "vertex.h"
#include "misc.h"
#include "uniforms.h"

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

static void draw_tri_tex_derivative(
    uint8_t *pixels, float *depth, int width, int height,
    Vertex v1, Vertex v2, Vertex v3,
    Vec3 albedo, const Uniforms *uni,
    const Texture *tex
)
{
    float area = edge(v1.scr, v2.scr, v3.scr);
    if (fabsf(area) < 1e-6f)
        return;
    if (uni->cull_backfaces && area < 0.f)
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

    float dalpha_dx = (v2.scr.y - v3.scr.y) * inv_area;
    float dalpha_dy = (v3.scr.x - v2.scr.x) * inv_area;
    float dbeta_dx  = (v3.scr.y - v1.scr.y) * inv_area;
    float dbeta_dy  = (v1.scr.x - v3.scr.x) * inv_area;
    float dgamma_dx = (v1.scr.y - v2.scr.y) * inv_area;
    float dgamma_dy = (v2.scr.x - v1.scr.x) * inv_area;
    
    float uiw1 = v1.uv.x * v1.invw, uiw2 = v2.uv.x * v2.invw, uiw3 = v3.uv.x * v3.invw;
    float viw1 = v1.uv.y * v1.invw, viw2 = v2.uv.y * v2.invw, viw3 = v3.uv.y * v3.invw;
    
    float iw1 = v1.invw, iw2 = v2.invw, iw3 = v3.invw;

    float dA_dx  = dalpha_dx * iw1 + dbeta_dx * iw2 + dgamma_dx * iw3;
    float dA_dy  = dalpha_dy * iw1 + dbeta_dy * iw2 + dgamma_dy * iw3;
    float dBu_dx = dalpha_dx * uiw1 + dbeta_dx * uiw2 + dgamma_dx * uiw3;
    float dBu_dy = dalpha_dy * uiw1 + dbeta_dy * uiw2 + dgamma_dy * uiw3;
    float dBv_dx = dalpha_dx * viw1 + dbeta_dx * viw2 + dgamma_dx * viw3;
    float dBv_dy = dalpha_dy * viw1 + dbeta_dy * viw2 + dgamma_dy * viw3;

    float tw = 0.f, th = 0.f;
    if (tex && tex->levels)
    {
        tw = (float)tex->ws[0];
        th = (float)tex->hs[0];
    }

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

            float t = bu * iw1 + bv * iw2 + bw * iw3;
            float inv_t = 1.f / t;
            float Bu = (bu * uiw1 + bv * uiw2 + bw * uiw3);
            float Bv = (bu * viw1 + bv * viw2 + bw * viw3);
            float u = Bu * inv_t;
            float v = Bv * inv_t;

            Vec3 albedo_used;
            float lod;
            if (uni->shade_mode & SHADE_SOLID)
            {
                lod = 0.f;
                albedo_used = albedo;
            }
            else if (tex && tex->levels)
            {
                float du_dx = (dBu_dx - u * dA_dx) * inv_t;
                float dv_dx = (dBv_dx - v * dA_dx) * inv_t;
                float du_dy = (dBu_dy - u * dA_dy) * inv_t;
                float dv_dy = (dBv_dy - v * dA_dy) * inv_t;

                float rho_x2 = (du_dx * tw) * (du_dx * tw) + (dv_dx * th) * (dv_dx * th);
                float rho_y2 = (du_dy * tw) * (du_dy * tw) + (dv_dy * th) * (dv_dy * th);
                float rho2 = rho_x2 > rho_y2 ? rho_x2 : rho_y2;

                lod = .5f * log2f(rho2) + uni->lod_bias;
                if (lod < 0.f)
                    lod = 0.f;
                if (lod > tex->count - 1)
                    lod = tex->count - 1;
                albedo_used = texture_sample_trilinear(tex, u, v, lod);
            }
            else
            {
                lod = 0.f;
                albedo_used = albedo;
            }

            depth[idx] = z;

            if (uni->shade_mode & SHADE_DEBUG)
            {
                Vec3 dbg = lod_debug_color(lod);
                pixels[idx << 2] = to_u8(dbg.x);
                pixels[(idx << 2) + 1] = to_u8(dbg.y);
                pixels[(idx << 2) + 2] = to_u8(dbg.z);
                pixels[(idx << 2) + 3] = 255;
                continue;
            }
            if (uni->shade_mode & SHADE_DEPTH_ONLY)
            {
                float lin = linear_depth_from_z_ndc(z, uni->near, uni->far);
                float t = clamp01((lin - uni->near) / (uni->far - uni->near));
                uint8_t g = to_u8(1.f - t);
                pixels[idx << 2] = g;
                pixels[(idx << 2) + 1] = g;
                pixels[(idx << 2) + 2] = g;
                pixels[(idx << 2) + 3] = 255;
                continue;
            }

            Vec3 n = {
                (bu * v1.n.x * v1.invw + bv * v2.n.x * v2.invw + bw * v3.n.x * v3.invw) * inv_t,
                (bu * v1.n.y * v1.invw + bv * v2.n.y * v2.invw + bw * v3.n.y * v3.invw) * inv_t,
                (bu * v1.n.z * v1.invw + bv * v2.n.z * v2.invw + bw * v3.n.z * v3.invw) * inv_t,
            };
            n = normalized3(n);

            float ndotl = dot3(n, uni->l_clip);
            if (ndotl < 0.f)
                ndotl = 0.f;
            float shade = ambient + diffuse * ndotl;

            pixels[idx << 2] = to_u8(albedo_used.x * shade);
            pixels[(idx << 2) + 1] = to_u8(albedo_used.y * shade);
            pixels[(idx << 2) + 2] = to_u8(albedo_used.z * shade);
            pixels[(idx << 2) + 3] = 255;
        }
    }
}

void draw_tri_tex(
    uint8_t *pixels, float *depth, int width, int height,
    Vertex v1, Vertex v2, Vertex v3,
    Vec3 albedo, const Uniforms *uni,
    const Texture *tex
)
{
    if (uni->shade_mode & SHADE_DERIVATIVE)
    {
        draw_tri_tex_derivative(
            pixels, depth, width, height,
            v1, v2, v3,
            albedo, uni,
            tex
        );
        return;
    }

    float area = edge(v1.scr, v2.scr, v3.scr);
    if (fabsf(area) < 1e-6f)
        return;
    if (uni->cull_backfaces && area < 0.f)
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

            float lod = 0.f;
            Vec3 albedo_used;
            if (uni->shade_mode & SHADE_SOLID)
            {
                lod = 0.f;
                albedo_used = albedo;
            }
            else if (tex && tex->levels)
            {
                lod = lod_by_depth(z, uni->near, uni->far, uni->tex_scale, uni->lod_bias);
                albedo_used = texture_sample_trilinear(tex, uv.x, uv.y, lod);
            }
            else
            {
                albedo_used = albedo;
            }

            depth[idx] = z;

            if (uni->shade_mode & SHADE_DEBUG)
            {
                Vec3 dbg = lod_debug_color(lod);
                pixels[idx << 2] = to_u8(dbg.x);
                pixels[(idx << 2) + 1] = to_u8(dbg.y);
                pixels[(idx << 2) + 2] = to_u8(dbg.z);
                pixels[(idx << 2) + 3] = 255;
                continue;
            }
            if (uni->shade_mode & SHADE_DEPTH_ONLY)
            {
                float lin = linear_depth_from_z_ndc(z, uni->near, uni->far);
                float t = clamp01((lin - uni->near) / (uni->far - uni->near));
                uint8_t g = to_u8(1.f - t);
                pixels[idx << 2] = g;
                pixels[(idx << 2) + 1] = g;
                pixels[(idx << 2) + 2] = g;
                pixels[(idx << 2) + 3] = 255;
                continue;
            }

            float ndotl = dot3(n, uni->l_clip);
            if (ndotl < 0.f)
                ndotl = 0.f;
            float shade = ambient + diffuse * ndotl;

            pixels[idx << 2] = to_u8(albedo_used.x * shade);
            pixels[(idx << 2) + 1] = to_u8(albedo_used.y * shade);
            pixels[(idx << 2) + 2] = to_u8(albedo_used.z * shade);
            pixels[(idx << 2) + 3] = 255;
        }
    }
}
