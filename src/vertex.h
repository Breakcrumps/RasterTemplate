#ifndef VERTEX_H
#define VERTEX_H

#include <stdbool.h>
#include "vectors.h"

typedef struct vertex
{
    Vec2 scr, uv;
    Vec3 n;
    float depth, invw;
} Vertex;

static inline Vertex make_vertex(Vec4 c, Vec3 n, Vec2 uv, int w, int h)
{
    float invw = 1.f / c.w;
    return (Vertex){
        .invw = invw,
        .n = n,
        .uv = uv,
        .scr = (Vec2){
            (c.x * invw * .5f + .5f) * w,
            (c.y * invw * .5f + .5f) * h
        },
        .depth = c.z * invw
    };
}

static int clip_near(
    const Vec4 cin[3], const Vec3 nin[3], const Vec2 uvin[3],
    float near,
    Vec4 cout[4], Vec3 nout[4], Vec2 uvout[4]
)
{
    int n = 0;
    for (int i = 0; i < 3; i++)
    {
        int j = (i + 1) % 3;
        bool a_in = cin[i].w >= near;
        bool b_in = cin[j].w >= near;

        if (a_in)
        {
            cout[n] = cin[i];
            nout[n] = nin[i];
            uvout[n] = uvin[i];
            n++;
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
            n++;
        }
    }
    return n;
}

#endif
