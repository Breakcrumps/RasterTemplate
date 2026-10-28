#ifndef MATRICES_H
#define MATRICES_H

#include <math.h>
#include "vectors.h"

typedef struct mat3x3
{
    union
    {
        struct { float r1[3], r2[3], r3[3]; };
        float m[3][3], items[9];
    };
} Mat3x3;
typedef struct mat4x4
{
    union
    {
        struct { float r1[4], r2[4], r3[4], r4[4]; };
        float m[4][4], items[16];
    };
} Mat4x4;

typedef enum axis { X, Y, Z, W } Axis;

static inline Vec3 mul_m3x3_v3(Mat3x3 mat, Vec3 vec)
{
    return (Vec3){
        mat.m[0][0] * vec.x + mat.m[0][1] * vec.y + mat.m[0][2] * vec.z,
        mat.m[1][0] * vec.x + mat.m[1][1] * vec.y + mat.m[1][2] * vec.z,
        mat.m[2][0] * vec.x + mat.m[2][1] * vec.y + mat.m[2][2] * vec.z
    };
}

static inline Vec4 mul_m4x4_v4(Mat4x4 mat, Vec4 vec)
{
    return (Vec4){
        mat.m[0][0] * vec.x + mat.m[0][1] * vec.y + mat.m[0][2] * vec.z + mat.m[0][3] * vec.w,
        mat.m[1][0] * vec.x + mat.m[1][1] * vec.y + mat.m[1][2] * vec.z + mat.m[1][3] * vec.w,
        mat.m[2][0] * vec.x + mat.m[2][1] * vec.y + mat.m[2][2] * vec.z + mat.m[2][3] * vec.w,
        mat.m[3][0] * vec.x + mat.m[3][1] * vec.y + mat.m[3][2] * vec.z + mat.m[3][3] * vec.w,
    };
}

static Mat3x3 mult3x3(Mat3x3 m1, Mat3x3 m2)
{
    float m00 = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0];
    float m01 = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1];
    float m02 = m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2];
    float m10 = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0];
    float m11 = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1];
    float m12 = m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2];
    float m20 = m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0];
    float m21 = m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1];
    float m22 = m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2];
    return (Mat3x3){
        m00, m01, m02,
        m10, m11, m12,
        m20, m21, m22
    };
}
static Mat4x4 mult4x4(Mat4x4 m1, Mat4x4 m2)
{
    float m00 = m1.m[0][0] * m2.m[0][0] + m1.m[0][1] * m2.m[1][0] + m1.m[0][2] * m2.m[2][0] + m1.m[0][3] * m2.m[3][0];
    float m01 = m1.m[0][0] * m2.m[0][1] + m1.m[0][1] * m2.m[1][1] + m1.m[0][2] * m2.m[2][1] + m1.m[0][3] * m2.m[3][1];
    float m02 = m1.m[0][0] * m2.m[0][2] + m1.m[0][1] * m2.m[1][2] + m1.m[0][2] * m2.m[2][2] + m1.m[0][3] * m2.m[3][2];
    float m03 = m1.m[0][0] * m2.m[0][3] + m1.m[0][1] * m2.m[1][3] + m1.m[0][2] * m2.m[2][3] + m1.m[0][3] * m2.m[3][3];
    float m10 = m1.m[1][0] * m2.m[0][0] + m1.m[1][1] * m2.m[1][0] + m1.m[1][2] * m2.m[2][0] + m1.m[1][3] * m2.m[3][0];
    float m11 = m1.m[1][0] * m2.m[0][1] + m1.m[1][1] * m2.m[1][1] + m1.m[1][2] * m2.m[2][1] + m1.m[1][3] * m2.m[3][1];
    float m12 = m1.m[1][0] * m2.m[0][2] + m1.m[1][1] * m2.m[1][2] + m1.m[1][2] * m2.m[2][2] + m1.m[1][3] * m2.m[3][2];
    float m13 = m1.m[1][0] * m2.m[0][3] + m1.m[1][1] * m2.m[1][3] + m1.m[1][2] * m2.m[2][3] + m1.m[1][3] * m2.m[3][3];
    float m20 = m1.m[2][0] * m2.m[0][0] + m1.m[2][1] * m2.m[1][0] + m1.m[2][2] * m2.m[2][0] + m1.m[2][3] * m2.m[3][0];
    float m21 = m1.m[2][0] * m2.m[0][1] + m1.m[2][1] * m2.m[1][1] + m1.m[2][2] * m2.m[2][1] + m1.m[2][3] * m2.m[3][1];
    float m22 = m1.m[2][0] * m2.m[0][2] + m1.m[2][1] * m2.m[1][2] + m1.m[2][2] * m2.m[2][2] + m1.m[2][3] * m2.m[3][2];
    float m23 = m1.m[2][0] * m2.m[0][3] + m1.m[2][1] * m2.m[1][3] + m1.m[2][2] * m2.m[2][3] + m1.m[2][3] * m2.m[3][3];
    float m30 = m1.m[3][0] * m2.m[0][0] + m1.m[3][1] * m2.m[1][0] + m1.m[3][2] * m2.m[2][0] + m1.m[3][3] * m2.m[3][0];
    float m31 = m1.m[3][0] * m2.m[0][1] + m1.m[3][1] * m2.m[1][1] + m1.m[3][2] * m2.m[2][1] + m1.m[3][3] * m2.m[3][1];
    float m32 = m1.m[3][0] * m2.m[0][2] + m1.m[3][1] * m2.m[1][2] + m1.m[3][2] * m2.m[2][2] + m1.m[3][3] * m2.m[3][2];
    float m33 = m1.m[3][0] * m2.m[0][3] + m1.m[3][1] * m2.m[1][3] + m1.m[3][2] * m2.m[2][3] + m1.m[3][3] * m2.m[3][3];
    return (Mat4x4){
        m00, m01, m02, m03,
        m10, m11, m12, m13,
        m20, m21, m22, m23,
        m30, m31, m32, m33
    };
}

static inline Mat3x3 transposed3x3(Mat3x3 mat)
{
    return (Mat3x3){
        mat.m[0][0], mat.m[1][0], mat.m[2][0],
        mat.m[0][1], mat.m[1][1], mat.m[2][1],
        mat.m[0][2], mat.m[1][2], mat.m[2][2]
    };
}
static inline Mat4x4 transposed4x4(Mat4x4 mat)
{
    return (Mat4x4){
        mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
        mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
        mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
        mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
    };
}

/// Возвращает 1 если успех, 0 иначе.
static int inv3x3(Mat3x3 *out, Mat3x3 mat)
{
    float det = mat.m[0][0] * (mat.m[1][1] * mat.m[2][2] - mat.m[2][1] * mat.m[1][2])
              - mat.m[0][1] * (mat.m[1][0] * mat.m[2][2] - mat.m[2][0] * mat.m[1][2])
              + mat.m[0][2] * (mat.m[1][0] * mat.m[2][1] - mat.m[2][0] * mat.m[1][1]);

    if (fabsf(det) < 1e-8f)
        return 0;
    float inv_det = 1.f / det;

    *out = (Mat3x3){
        (mat.m[1][1] * mat.m[2][2] - mat.m[1][2] * mat.m[2][1]) * inv_det,
        (mat.m[0][2] * mat.m[2][1] - mat.m[0][1] * mat.m[2][2]) * inv_det,
        (mat.m[0][1] * mat.m[1][2] - mat.m[0][2] * mat.m[1][1]) * inv_det,

        (mat.m[1][2] * mat.m[2][0] - mat.m[1][0] * mat.m[2][2]) * inv_det,
        (mat.m[0][0] * mat.m[2][2] - mat.m[0][2] * mat.m[2][0]) * inv_det,
        (mat.m[0][2] * mat.m[1][0] - mat.m[0][0] * mat.m[1][2]) * inv_det,

        (mat.m[1][0] * mat.m[2][1] - mat.m[1][1] * mat.m[2][0]) * inv_det,
        (mat.m[0][1] * mat.m[2][0] - mat.m[0][0] * mat.m[2][1]) * inv_det,
        (mat.m[0][0] * mat.m[1][1] - mat.m[0][1] * mat.m[1][0]) * inv_det,
    };
    return 1;
}
/// Возвращает 1 если успех, 0 иначе.
static int inv4x4(Mat4x4 *out, Mat4x4 mat)
{
    float s0 = mat.m[0][0] * mat.m[1][1] - mat.m[1][0] * mat.m[0][1];
    float s1 = mat.m[0][0] * mat.m[1][2] - mat.m[1][0] * mat.m[0][2];
    float s2 = mat.m[0][0] * mat.m[1][3] - mat.m[1][0] * mat.m[0][3];
    float s3 = mat.m[0][1] * mat.m[1][2] - mat.m[1][1] * mat.m[0][2];
    float s4 = mat.m[0][1] * mat.m[1][3] - mat.m[1][1] * mat.m[0][3];
    float s5 = mat.m[0][2] * mat.m[1][3] - mat.m[1][2] * mat.m[0][3];

    float c5 = mat.m[2][2] * mat.m[3][3] - mat.m[3][2] * mat.m[2][3];
    float c4 = mat.m[2][1] * mat.m[3][3] - mat.m[3][1] * mat.m[2][3];
    float c3 = mat.m[2][1] * mat.m[3][2] - mat.m[3][1] * mat.m[2][2];
    float c2 = mat.m[2][0] * mat.m[3][3] - mat.m[3][0] * mat.m[2][3];
    float c1 = mat.m[2][0] * mat.m[3][2] - mat.m[3][0] * mat.m[2][2];
    float c0 = mat.m[2][0] * mat.m[3][1] - mat.m[3][0] * mat.m[2][1];

    float det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    if (fabsf(det) < 1e-8f)
        return 0;
    float inv_det = 1.0f / det;

    *out = (Mat4x4){
        (mat.m[1][1] * c5 - mat.m[1][2] * c4 + mat.m[1][3] * c3) * inv_det,
        (-mat.m[0][1] * c5 + mat.m[0][2] * c4 - mat.m[0][3] * c3) * inv_det,
        (mat.m[3][1] * s5 - mat.m[3][2] * s4 + mat.m[3][3] * s3) * inv_det,
        (-mat.m[2][1] * s5 + mat.m[2][2] * s4 - mat.m[2][3] * s3) * inv_det,
    
        (-mat.m[1][0] * c5 + mat.m[1][2] * c2 - mat.m[1][3] * c1) * inv_det,
        (mat.m[0][0] * c5 - mat.m[0][2] * c2 + mat.m[0][3] * c1) * inv_det,
        (-mat.m[3][0] * s5 + mat.m[3][2] * s2 - mat.m[3][3] * s1) * inv_det,
        (mat.m[2][0] * s5 - mat.m[2][2] * s2 + mat.m[2][3] * s1) * inv_det,
    
        (mat.m[1][0] * c4 - mat.m[1][1] * c2 + mat.m[1][3] * c0) * inv_det,
        (-mat.m[0][0] * c4 + mat.m[0][1] * c2 - mat.m[0][3] * c0) * inv_det,
        (mat.m[3][0] * s4 - mat.m[3][1] * s2 + mat.m[3][3] * s0) * inv_det,
        (-mat.m[2][0] * s4 + mat.m[2][1] * s2 - mat.m[2][3] * s0) * inv_det,
    
        (-mat.m[1][0] * c3 + mat.m[1][1] * c1 - mat.m[1][2] * c0) * inv_det,
        (mat.m[0][0] * c3 - mat.m[0][1] * c1 + mat.m[0][2] * c0) * inv_det,
        (-mat.m[3][0] * s3 + mat.m[3][1] * s1 - mat.m[3][2] * s0) * inv_det,
        (mat.m[2][0] * s3 - mat.m[2][1] * s1 + mat.m[2][2] * s0) * inv_det
    };

    return 1;
}

static inline Mat3x3 translate_m3x3(float dx, float dy)
{
    return (Mat3x3){
        1., 0., dx,
        0., 1., dy,
        0., 0., 1.
    };
}
static inline Mat4x4 translate_m4x4(float dx, float dy, float dz)
{
    return (Mat4x4){
        1., 0., 0., dx,
        0., 1., 0., dy,
        0., 0., 1., dz,
        0., 0., 0., 1.
    };
}

static inline Mat3x3 rot_m3x3(float angle, Axis axis)
{
    float c = cosf(angle), s = sinf(angle);
    return (
        axis == X ? (Mat3x3){
            1., 0., 0.,
            0., c, -s,
            0., s, c
        }
        : axis == Y ? (Mat3x3){
            c, 0., s,
            0., 1., 0.,
            -s, 0., c
        }
        : axis == Z ? (Mat3x3){
            c, -s, 0.,
            s, c, 0.,
            0., 0., 1.
        }
        : (Mat3x3){
            1., 0., 0.,
            0., 1., 0.,
            0., 0., 1.
        }
    );
}
static inline Mat4x4 rot_m4x4(float angle, Axis axis)
{
    float c = cosf(angle), s = sinf(angle);
    return (
        axis == X ? (Mat4x4){
            1., 0., 0., 0.,
            0., c,-s, 0.,
            0., s, c, 0.,
            0., 0., 0., 1.
        }
        : axis == Y ? (Mat4x4){
            c, 0., s, 0.,
            0., 1., 0., 0.,
            -s, 0., c, 0.,
            0., 0., 0., 1.
        }
        : axis == Z ? (Mat4x4){
            c,-s, 0., 0.,
            s, c, 0., 0.,
            0., 0., 1., 0.,
            0., 0., 0., 1.
        }
        : (Mat4x4){
            1., 0., 0., 0.,
            0., 1., 0., 0.,
            0., 0., 1., 0.,
            0., 0., 0., 1.
        }
    );
}

static inline Mat3x3 scale_m3x3(float sx, float sy, float sz)
{
    return (Mat3x3){
        sx, 0., 0.,
        0., sy, 0.,
        0., 0., sz
    };
}
static inline Mat4x4 scale_m4x4(float sx, float sy, float sz, float sw)
{
    return (Mat4x4){
        sx, 0., 0., 0.,
        0., sy, 0., 0.,
        0., 0., sz, 0.,
        0., 0., 0., sw,
    };
}

static inline Mat3x3 look_at_m3x3(Vec3 eyepos, Vec3 targetpos, Vec3 updir)
{
    Vec3 f = normalized3(sub3(targetpos, eyepos));
    Vec3 r = normalized3(cross3(f, updir));
    Vec3 u = cross3(r, f);
    return (Mat3x3){
        r.x, r.y, r.z,
        u.x, u.y, u.z,
        -f.x, -f.y, -f.z,
    };
}
static inline Mat4x4 look_at_m4x4(Vec3 eyepos, Vec3 targetpos, Vec3 updir)
{
    Vec3 f = normalized3(sub3(targetpos, eyepos));
    Vec3 r = normalized3(cross3(f, updir));
    Vec3 u = cross3(r, f);
    return (Mat4x4){
        r.x, r.y, r.z, -dot3(r, eyepos),
        u.x, u.y, u.z, -dot3(u, eyepos),
        -f.x, -f.y, -f.z, dot3(f, eyepos),
        0, 0, 0, 1
    };
}

static inline Mat4x4 perspective_m4x4(float fov_y, float aspect, float near, float far) {
    if (near <= 0.f || far <= near || aspect <= 0.f)
        return (Mat4x4){
            1., 0., 0., 0.,
            0., 1., 0., 0.,
            0., 0., 1., 0.,
            0., 0., 0., 1.
        };
    float f = 1.f / tanf(fov_y * .5f);
    float nf = 1.f / (near - far);
    return (Mat4x4){{
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (far + near) * nf, 2 * far * near * nf,
        0, 0, -1, 0,
    }};
}

static inline Mat4x4 trs_m4x4(float angle, Axis axis, Vec3 translate, float scale)
{
    Mat4x4 r = rot_m4x4(angle, axis);
    Mat4x4 s = scale_m4x4(scale, scale, scale, 1.f);
    Mat4x4 t = translate_m4x4(translate.x, translate.y, translate.z);
    return mult4x4(t, mult4x4(r, s));
}

#endif
