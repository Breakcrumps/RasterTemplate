#include "terrain.h"
#include "misc.h"
#include "uniforms.h"
#include "texture.h"
#include "image_utils.h"
#include "vertex.h"
#include "noise.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

void terrain_init(
    Terrain *t, int grid_n, float step,
    float height_scale,
    unsigned int seed, int octaves
)
{
    t->grid_n = grid_n;
    t->step = step;
    t->hm_w = t->hm_h = grid_n << 1;
    t->world_size = grid_n * step;
    t->height_scale = height_scale;

    t->heightmap = malloc(t->hm_w * t->hm_h * sizeof(float));
    float noise_scale = t->world_size / 8.f;
    float hm_step = t->world_size / t->hm_w;
    for (int j = 0; j < t->hm_w; j++)
    {
        for (int i = 0; i < t->hm_h; i++)
        {
            float wx = i * hm_step;
            float wz = j * hm_step;
            t->heightmap[j * t->hm_w + i] = fbm2d(
                wx / noise_scale, wz / noise_scale,
                octaves, 2.f, .5f, seed
            );
        }
    }

    int nv = (grid_n + 1) * (grid_n + 1);
    t->cache = malloc(nv * sizeof(TerrainVert));
}

void terrain_free(Terrain *t)
{
    free(t->heightmap);
    free(t->cache);
    memset(t, 0, sizeof(Terrain));
}

void terrain_save_heightmap_png(const Terrain *t, const char *path)
{
    int n = t->hm_w * t->hm_h;
    float mn = t->heightmap[0], mx = t->heightmap[0];
    for (int i = 1; i < n; i++)
    {
        if (t->heightmap[i] < mn)
            mn = t->heightmap[i];
        if (t->heightmap[i] > mx)
            mx = t->heightmap[i];
    }
    float range = mx - mn;
    if (range < 1e-6f)
        range = 1.f;
    float *rgb = malloc(n * 3 * sizeof(float));
    for (int i = 0; i < n; i++)
    {
        float v = (t->heightmap[i] - mn) / range;
        rgb[i * 3] = rgb[i * 3 + 1] = rgb[i * 3 + 2] = v;
    }
    save_image_f32_png_rgb(rgb, path, t->hm_w, t->hm_h, 3, 1.f);
    free(rgb);
}

static float terrain_sample_height(const Terrain *t, float wx, float wz)
{
    float hx = wx * t->hm_w / t->world_size;
    float hz = wz * t->hm_h / t->world_size;

    hx = hx - floorf(hx / t->hm_w) * t->hm_w;
    hz = hz - floorf(hz / t->hm_h) * t->hm_h;
    if (hx >= t->hm_w)
        hx = 0.f;
    if (hz >= t->hm_h)
        hz = 0.f;

    int x0 = floorf(hx), z0 = floorf(hz);
    int x1 = (x0 + 1) % t->hm_w;
    int z1 = (z0 + 1) % t->hm_h;
    float fx = hx - x0;
    float fz = hz - z0;

    float h00 = t->heightmap[z0 * t->hm_w + x0];
    float h10 = t->heightmap[z0 * t->hm_w + x1];
    float h01 = t->heightmap[z1 * t->hm_w + x0];
    float h11 = t->heightmap[z1 * t->hm_w + x1];

    float h = h00 * (1 - fx) * (1 - fz) + h10 * fx * (1 - fz)
            + h01 * (1 - fx) * fz + h11 * fx * fz;
    return h * t->height_scale;
}
float terrain_surface_height(const Terrain *t, float wx, float wz)
{
    float step = t->step;
    float half = (t->grid_n >> 1) * step;

    float origin_x = roundf(wx / step) * step - half;
    float origin_z = roundf(wz / step) * step - half;

    float hx = (wx - origin_x) / step;
    float hz = (wz - origin_z) / step;
    int i = floorf(hx);
    int j = floorf(hz);
    if (i < 0)
        i = 0;
    if (j < 0)
        j = 0;
    if (i > t->grid_n - 1)
        i = t->grid_n - 1;
    if (j > t->grid_n - 1)
        j = t->grid_n - 1;
    float fx = hx - i;
    float fz = hz - j;

    float x0 = origin_x + i * step;
    float z0 = origin_z + j * step;
    float x1 = x0 + step;
    float z1 = z0 + step;

    float h00 = terrain_sample_height(t, x0, z0);
    float h10 = terrain_sample_height(t, x1, z0);
    float h01 = terrain_sample_height(t, x0, z1);
    float h11 = terrain_sample_height(t, x1, z1);

    if (fx + fz <= 1.f)
        return h00 + fx * (h10 - h00) + fz * (h01 - h00);
    else
        return h11 * (fx + fz - 1.f) + h10 * (1.f - fz) + h01 * (1.f - fx);
}

Vec3 terrain_sample_normal(const Terrain *t, float wx, float wz)
{
    float eps = t->step;
    float hL = terrain_sample_height(t, wx - eps, wz);
    float hR = terrain_sample_height(t, wx + eps, wz);
    float hD = terrain_sample_height(t, wx, wz - eps);
    float hU = terrain_sample_height(t, wx, wz + eps);
    Vec3 n = { (hL - hR) / (2.f * eps), 1.f, (hD - hU) / (2.f * eps) };
    return normalized3(n);
}

static int terrain_clip_near(
    const Vec4 cin[3], const Vec2 uvin[3], const Vec3 nin[3], const float bin[3],
    float near,
    Vec4 cout[4], Vec2 uvout[4], Vec3 nout[4], float bout[4]
)
{
    int n = 0;
    for (int i = 0; i < 3; i++)
    {
        int j = (i + 1) % 3;
        int a_in = cin[i].w >= near;
        int b_in = cin[j].w >= near;
        if (a_in)
        {
            cout[n] = cin[i]; uvout[n] = uvin[i];
            nout[n] = nin[i]; bout[n] = bin[i]; n++;
        }
        if (a_in != b_in)
        {
            float t = (near - cin[i].w) / (cin[j].w - cin[i].w);
            cout[n] = (Vec4){
                cin[i].x + t * (cin[j].x - cin[i].x),
                cin[i].y + t * (cin[j].y - cin[i].y),
                cin[i].z + t * (cin[j].z - cin[i].z),
                near
            };
            nout[n] = normalized3((Vec3){
                nin[i].x + t * (nin[j].x - nin[i].x),
                nin[i].y + t * (nin[j].y - nin[i].y),
                nin[i].z + t * (nin[j].z - nin[i].z)
            });
            uvout[n] = (Vec2){
                uvin[i].x + t * (uvin[j].x - uvin[i].x),
                uvin[i].y + t * (uvin[j].y - uvin[i].y)
            };
            bout[n] = bin[i] + t * (bin[j] - bin[i]);
            n++;
        }
    }
    return n;
}

static void draw_tri_terrain(
    uint8_t *pixels, float *depth, int width, int height,
    Vec4 c[3], Vec2 uv[3], Vec3 nrm[3], float blend[3],
    const Uniforms *uni,
    const Texture *grass, const Texture *rock
)
{
    if (c[0].w <= 0.f && c[1].w <= 0.f && c[2].w <= 0.f)
        return;

    Vec2 scr[3];
    float invw[3], zval[3];
    for (int i = 0; i < 3; i++)
    {
        invw[i] = 1.f / c[i].w;
        scr[i].x = (c[i].x * invw[i] * .5f + .5f) * width;
        scr[i].y = (c[i].y * invw[i] * .5f + .5f) * height;
        zval[i] = c[i].z * invw[i];
    }

    int min_x = fminf(scr[0].x, fminf(scr[1].x, scr[2].x));
    int max_x = fmaxf(scr[0].x, fmaxf(scr[1].x, scr[2].x));
    int min_y = fminf(scr[0].y, fminf(scr[1].y, scr[2].y));
    int max_y = fmaxf(scr[0].y, fmaxf(scr[1].y, scr[2].y));
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

    float area = edge(scr[0], scr[1], scr[2]);
    if (fabsf(area) < 1e-6f)
        return;
    if (area < 0.f)
    {
        Vec2 t2 = scr[1]; scr[1] = scr[2]; scr[2] = t2;
        Vec2 tu = uv[1]; uv[1] = uv[2]; uv[2] = tu;
        Vec3 tn = nrm[1]; nrm[1] = nrm[2]; nrm[2] = tn;
        float tb = blend[1]; blend[1] = blend[2]; blend[2] = tb;
        float ti = invw[1]; invw[1] = invw[2]; invw[2] = ti;
        float tz = zval[1]; zval[1] = zval[2]; zval[2] = tz;
        area = -area;
    }
    float inv_area = 1.f / area;

    float dax = (scr[1].y - scr[2].y) * inv_area;
    float day = (scr[2].x - scr[1].x) * inv_area;
    float dbx = (scr[2].y - scr[0].y) * inv_area;
    float dby = (scr[0].x - scr[2].x) * inv_area;
    float dgx = (scr[0].y - scr[1].y) * inv_area;
    float dgy = (scr[1].x - scr[0].x) * inv_area;

    float uiw1 = uv[0].x * invw[0], uiw2 = uv[1].x * invw[1], uiw3 = uv[2].x * invw[2];
    float viw1 = uv[0].y * invw[0], viw2 = uv[1].y * invw[1], viw3 = uv[2].y * invw[2];
    float biw1 = blend[0] * invw[0], biw2 = blend[1] * invw[1], biw3 = blend[2] * invw[2];
    float iw1 = invw[0], iw2 = invw[1], iw3 = invw[2];

    float dA_dx  = dax * iw1 + dbx * iw2 + dgx * iw3;
    float dA_dy  = day * iw1 + dby * iw2 + dgy * iw3;
    float dBu_dx = dax * uiw1 + dbx * uiw2 + dgx * uiw3;
    float dBu_dy = day * uiw1 + dby * uiw2 + dgy * uiw3;
    float dBv_dx = dax * viw1 + dbx * viw2 + dgx * viw3;
    float dBv_dy = day * viw1 + dby * viw2 + dgy * viw3;

    float tw = grass->levels ? grass->ws[0] : 1.f;
    float th = grass->levels ? grass->hs[0] : 1.f;

    for (int y = min_y; y <= max_y; y++)
    {
        for (int x = min_x; x <= max_x; x++)
        {
            size_t idx = (size_t)y * width + x;
            
            Vec2 p = { x + .5f, y + .5f };
            float bu = edge(scr[1], scr[2], p) * inv_area;
            float bv = edge(scr[2], scr[0], p) * inv_area;
            float bw = edge(scr[0], scr[1], p) * inv_area;
            if (bu < 0.f || bv < 0.f || bw < 0.f)
                continue;

            float z = bu * zval[0] + bv * zval[1] + bw * zval[2];
            if (z >= depth[idx])
                continue;

            float t = bu * iw1 + bv * iw2 + bw * iw3;
            float inv_t = 1.f / t;
            float Bu = bu * uiw1 + bv * uiw2 + bw * uiw3;
            float Bv = bu * viw1 + bv * viw2 + bw * viw3;
            float u  = Bu * inv_t;
            float vv = Bv * inv_t;
            float bl = (bu * biw1 + bv * biw2 + bw * biw3) * inv_t;

            Vec3 n = {
                (bu * nrm[0].x * iw1 + bv * nrm[1].x * iw2 + bw * nrm[2].x * iw3) * inv_t,
                (bu * nrm[0].y * iw1 + bv * nrm[1].y * iw2 + bw * nrm[2].y * iw3) * inv_t,
                (bu * nrm[0].z * iw1 + bv * nrm[1].z * iw2 + bw * nrm[2].z * iw3) * inv_t,
            };
            n = normalized3(n);

            float du_dx = (dBu_dx - u * dA_dx) * inv_t;
            float dv_dx = (dBv_dx - vv * dA_dx) * inv_t;
            float du_dy = (dBu_dy - u * dA_dy) * inv_t;
            float dv_dy = (dBv_dy - vv * dA_dy) * inv_t;
            float rx2 = (du_dx * tw) * (du_dx * tw) + (dv_dx * th) * (dv_dx * th);
            float ry2 = (du_dy * tw) * (du_dy * tw) + (dv_dy * th) * (dv_dy * th);
            float rho2 = rx2 > ry2 ? rx2 : ry2;
            float lod = .5f * log2f(rho2 > 1e-12f ? rho2 : 1e-12f) + uni->lod_bias;
            if (lod < 0.f)
                lod = 0.f;

            depth[idx] = z;

            if (uni->shade_mode & SHADE_LOD)
            {
                Vec3 dbg = lod_debug_color(lod);
                pixels[idx << 2] = to_u8(dbg.x);
                pixels[(idx << 2) + 1] = to_u8(dbg.y);
                pixels[(idx << 2) + 2] = to_u8(dbg.z);
                pixels[(idx << 2) + 3] = 255;
                continue;
            }

            Vec3 cg = texture_sample_trilinear(grass, u, vv, lod);
            Vec3 cr = texture_sample_trilinear(rock,  u, vv, lod);
            Vec3 albedo;
            if (uni->shade_mode & SHADE_SOLID)
                albedo = make_v3(.55f, .5f, .45f);
            else
                albedo = make_v3(
                    cg.x * (1.f - bl) + cr.x * bl,
                    cg.y * (1.f - bl) + cr.y * bl,
                    cg.z * (1.f - bl) + cr.z * bl
                );

            float ndotl = dot3(n, uni->l_clip);
            if (ndotl < 0.f)
                ndotl = 0.f;
            float shade = .05f + .95f * ndotl;

            pixels[idx << 2] = to_u8(albedo.x * shade);
            pixels[(idx << 2) + 1] = to_u8(albedo.y * shade);
            pixels[(idx << 2) + 2] = to_u8(albedo.z * shade);
            pixels[(idx << 2) + 3] = 255;
        }
    }
}

static void terrain_emit_tri(
    uint8_t *pixels, float *depth, int width, int height,
    const TerrainVert *a, const TerrainVert *b, const TerrainVert *c,
    float near, const Uniforms *uni,
    const Texture *grass, const Texture *rock
)
{
    Vec4 cin[3] = { a->clip, b->clip, c->clip };
    Vec2 uvin[3] = { a->uv, b->uv, c->uv };
    Vec3 nin[3]  = { a->n_view, b->n_view, c->n_view };
    float bin[3] = { a->blend, b->blend, c->blend };

    Vec4 cout[4]; Vec2 uvout[4]; Vec3 nout[4]; float bout[4];
    int n = terrain_clip_near(
        cin, uvin, nin, bin, near,
        cout, uvout, nout, bout
    );
    if (n < 3)
        return;

    for (int k = 1; k + 1 < n; k++)
    {
        Vec4 cc[3] = { cout[0], cout[k], cout[k + 1] };
        Vec2 uc[3] = { uvout[0], uvout[k], uvout[k + 1] };
        Vec3 nc[3] = { nout[0], nout[k], nout[k + 1] };
        float bc[3] = { bout[0], bout[k], bout[k + 1] };
        draw_tri_terrain(
            pixels, depth, width, height,
            cc, uc, nc, bc, uni, grass, rock
        );
    }
}

void terrain_draw(
    uint8_t *pixels, float *depth, int width, int height,
    Terrain *t, Vec3 camera_pos,
    const Uniforms *uni,
    const Texture *grass, const Texture *rock)
{
    int n = t->grid_n;
    float step = t->step;
    float half = (n >> 1) * step;

    float snap_x = roundf(camera_pos.x / step) * step;
    float snap_z = roundf(camera_pos.z / step) * step;
    float origin_x = snap_x - half;
    float origin_z = snap_z - half;

    Mat3x3 view3 = {
        uni->view.m[0][0], uni->view.m[0][1], uni->view.m[0][2],
        uni->view.m[1][0], uni->view.m[1][1], uni->view.m[1][2],
        uni->view.m[2][0], uni->view.m[2][1], uni->view.m[2][2],
    };

    const float uv_scale = .25f;
    int stride = n + 1;

    for (int j = 0; j <= n; j++)
    {
        for (int i = 0; i <= n; i++)
        {
            float wx = origin_x + i * step;
            float wz = origin_z + j * step;
            float h  = terrain_sample_height(t, wx, wz);
            Vec3 nw  = terrain_sample_normal(t, wx, wz);
            Vec3 nv  = normalized3(mul_m3x3_v3(view3, nw));
            Vec4 clip = mul_m4x4_v4(uni->vp, make_v4(wx, h, wz, 1.f));
            Vec2 uv = { wx * uv_scale, wz * uv_scale };
            float blend = clamp01((.75f - nw.y) / .45f);
            t->cache[j * stride + i] = (TerrainVert){ clip, nv, uv, blend };
        }
    }

    for (int j = 0; j < n; j++)
    {
        for (int i = 0; i < n; i++)
        {
            int i00 = j * stride + i;
            int i10 = i00 + 1;
            int i01 = i00 + stride;
            int i11 = i01 + 1;
            terrain_emit_tri(
                pixels, depth, width, height,
                &t->cache[i00], &t->cache[i10], &t->cache[i01],
                uni->near, uni, grass, rock
            );
            terrain_emit_tri(
                pixels, depth, width, height,
                &t->cache[i10], &t->cache[i11], &t->cache[i01],
                uni->near, uni, grass, rock
            );
        }
    }
}
