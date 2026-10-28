#include "mesh_utils.h"

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "external/tinyobj_loader_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#ifdef _WIN64
#define atoll(S) _atoi64(S)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

static char* mmap_file(size_t* len, const char* filename) {
#ifdef _WIN64
  HANDLE file =
      CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);

  if (file == INVALID_HANDLE_VALUE) { /* E.g. Model may not have materials. */
    return NULL;
  }

  HANDLE fileMapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
  assert(fileMapping != INVALID_HANDLE_VALUE);

  LPVOID fileMapView = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
  char* fileMapViewChar = (char*)fileMapView;
  assert(fileMapView != NULL);

  DWORD file_size = GetFileSize(file, NULL);
  (*len) = (size_t)file_size;

  return fileMapViewChar;
#else

  struct stat sb;
  char* p;
  int fd;

  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    perror("open");
    return NULL;
  }

  if (fstat(fd, &sb) == -1) {
    perror("fstat");
    return NULL;
  }

  if (!S_ISREG(sb.st_mode)) {
    fprintf(stderr, "%s is not a file\n", filename);
    return NULL;
  }

  p = (char*)mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

  if (p == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }

  if (close(fd) == -1) {
    perror("close");
    return NULL;
  }

  (*len) = sb.st_size;

  return p;

#endif
}

static void get_file_data(void* ctx, const char* filename, const int is_mtl,
                          const char* obj_filename, char** data, size_t* len) 
                          {
  // NOTE: If you allocate the buffer with malloc(),
  // You can define your own memory management struct and pass it through `ctx`
  // to store the pointer and free memories at clean up stage(when you quit an
  // app)
  // This example uses mmap(), so no free() required.
  (void)ctx;

  if (!filename) {
    fprintf(stderr, "null filename\n");
    (*data) = NULL;
    (*len) = 0;
    return;
  }

  size_t data_len = 0;

  *data = mmap_file(&data_len, filename);
  (*len) = data_len;
}

typedef struct
{
    int v_idx, vt_idx, vn_idx;
    int id; // -1 - empty
} vertex_slot;

static uint32_t hash_vertex(int v_idx, int vt_idx, int vn_idx)
{
    uint32_t h = (uint32_t)v_idx * 73856093u;
    h ^= (uint32_t)vt_idx * 19349663u;
    h ^= (uint32_t)vn_idx * 83492791u;
    return h;
}

static int valid_index(int idx, unsigned int count)
{
    return idx >= 0 && (unsigned int)idx < count;
}

Mesh load_obj(const char *filename)
{
    Mesh m;
    m.tri_count = 0;
    m.vert_count = 0;
    m.verts = NULL;
    m.normals = NULL;
    m.tcs = NULL;
    m.indices = NULL;

    tinyobj_attrib_t attrib;
    tinyobj_shape_t *shapes = NULL;
    size_t num_shapes;
    tinyobj_material_t *materials = NULL;
    size_t num_materials;

    unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
    int ret =
        tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials,
                          &num_materials, filename, get_file_data, NULL, flags);
    if (ret != TINYOBJ_SUCCESS)
        return m;

    const int num_corners = attrib.num_faces;

    int has_normals = attrib.num_normals > 0;
    int has_tcs = attrib.num_texcoords > 0;
    int valid = num_corners % 3 == 0;
    for (int i = 0; i < num_corners && valid; i++)
    {
        valid = valid_index(attrib.faces[i].v_idx, attrib.num_vertices);
        has_normals = has_normals && valid_index(attrib.faces[i].vn_idx, attrib.num_normals);
        has_tcs = has_tcs && valid_index(attrib.faces[i].vt_idx, attrib.num_texcoords);
    }

    if (valid)
    {
        m.verts = malloc(num_corners * sizeof(Vec3));
        m.normals = has_normals ? malloc(num_corners * sizeof(Vec3)) : NULL;
        m.tcs = has_tcs ? malloc(num_corners * sizeof(Vec2)) : NULL;
        m.indices = malloc(num_corners * sizeof(int));
        m.tri_count = num_corners/3;

        uint32_t table_size = 1;
        while (table_size < 2 * (uint32_t)num_corners)
            table_size *= 2;
        vertex_slot *table = malloc(table_size * sizeof(vertex_slot));
        for (uint32_t i = 0; i < table_size; i++)
            table[i].id = -1;

        for (int i = 0; i < num_corners; i++)
        {
            const int v_idx = attrib.faces[i].v_idx;
            const int vt_idx = has_tcs ? attrib.faces[i].vt_idx : -1;
            const int vn_idx = has_normals ? attrib.faces[i].vn_idx : -1;

            uint32_t slot = hash_vertex(v_idx, vt_idx, vn_idx) & (table_size - 1);
            while (table[slot].id >= 0 &&
                   !(table[slot].v_idx == v_idx && table[slot].vt_idx == vt_idx && table[slot].vn_idx == vn_idx))
                slot = (slot + 1) & (table_size - 1);

            if (table[slot].id < 0)
            {
                const int id = m.vert_count++;
                table[slot].v_idx = v_idx;
                table[slot].vt_idx = vt_idx;
                table[slot].vn_idx = vn_idx;
                table[slot].id = id;

                m.verts[id] = make_v3(attrib.vertices[3 * v_idx + 0], attrib.vertices[3 * v_idx + 1], attrib.vertices[3 * v_idx + 2]);
                if (has_normals)
                    m.normals[id] = make_v3(attrib.normals[3 * vn_idx + 0], attrib.normals[3 * vn_idx + 1], attrib.normals[3 * vn_idx + 2]);
                if (has_tcs)
                    m.tcs[id] = make_v2(attrib.texcoords[2 * vt_idx + 0], attrib.texcoords[2 * vt_idx + 1]);
            }

            m.indices[i] = table[slot].id;
        }

        free(table);

        m.verts = realloc(m.verts, m.vert_count * sizeof(Vec3));
        if (m.normals)
            m.normals = realloc(m.normals, m.vert_count * sizeof(Vec3));
        if (m.tcs)
            m.tcs = realloc(m.tcs, m.vert_count * sizeof(Vec2));
    }
    else
    {
        fprintf(stderr, "load_obj: %s has invalid face indices\n", filename);
    }

    tinyobj_attrib_free(&attrib);
    tinyobj_shapes_free(shapes, num_shapes);
    tinyobj_materials_free(materials, num_materials);
    return m;
}

void mesh_normalize(Mesh m)
{
    if (m.vert_count <= 0)
        return;

    Vec3 min = m.verts[0], max = m.verts[0];
    for (int i = 1; i < m.vert_count; i++)
    {
        Vec3 p = m.verts[i];
        if (p.x < min.x)
            min.x = p.x;
        if (p.y < min.y)
            min.y = p.y;
        if (p.z < min.z)
            min.z = p.z;
        if (p.x > max.x)
            max.x = p.x;
        if (p.y > max.y)
            max.y = p.y;
        if (p.z > max.z)
            max.z = p.z;
    }

    float cx = (min.x + max.x) * .5f;
    float cy = (min.y + max.y) * .5f;
    float cz = (min.z + max.z) * .5f;

    float ex = max.x - min.x, ey = max.y - min.y, ez = max.z - min.z;
    float max_extent = fmaxf(ex, fmaxf(ey, ez));
    float scale = (max_extent > 1e-8f) ? (1.f / max_extent) : 1.f;

    for (int i = 0; i < m.vert_count; i++)
    {
        m.verts[i].x = (m.verts[i].x - cx) * scale;
        m.verts[i].y = (m.verts[i].y - cy) * scale;
        m.verts[i].z = (m.verts[i].z - cz) * scale;
    }
}

void mesh_compute_normals(Mesh *m)
{
    if (m->normals || m->vert_count <= 0 || m->tri_count <= 0)
        return;

    m->normals = calloc(m->vert_count, sizeof(Vec3));
    if (!m->normals)
        return;

    for (int t = 0; t < m->tri_count; t++)
    {
        int i0 = m->indices[3 * t];
        int i1 = m->indices[3 * t + 1];
        int i2 = m->indices[3 * t + 2];

        Vec3 p0 = m->verts[i0];
        Vec3 p1 = m->verts[i1];
        Vec3 p2 = m->verts[i2];

        Vec3 n = cross3(sub3(p1, p0), sub3(p2, p0));

        m->normals[i0].x += n.x; m->normals[i0].y += n.y; m->normals[i0].z += n.z;
        m->normals[i1].x += n.x; m->normals[i1].y += n.y; m->normals[i1].z += n.z;
        m->normals[i2].x += n.x; m->normals[i2].y += n.y; m->normals[i2].z += n.z;
    }

    for (int i = 0; i < m->vert_count; i++)
    {
        m->normals[i] = normalized3(m->normals[i]);
    }
}
