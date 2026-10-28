#ifndef MESH_UTILS_H
#define MESH_UTILS_H
#include "vectors.h"

#include <stdlib.h>

// Треугольный меш. Все атрибуты индексируются одним индексом вершины:
// verts[i], normals[i], tcs[i]. Треугольник t - вершины indices[3*t+0..2].
//
// Нормали и текстурные координаты не генерируются: если в файле их нет (или они
// заданы не для всех вершин), normals / tcs равны NULL.
typedef struct mesh
{
    Vec3 *verts;
    Vec3 *normals;
    Vec2 *tcs;
    int *indices;
    int vert_count, tri_count;
} Mesh;

static inline void free_mesh(Mesh m)
{
    free(m.verts);
    free(m.normals);
    free(m.tcs);
    free(m.indices);
}
// При ошибке возвращает пустой меш (num_vertices = num_triangles = 0)
Mesh load_obj(const char *filename);

void mesh_normalize(Mesh m);

void mesh_compute_normals(Mesh *m);

#endif
