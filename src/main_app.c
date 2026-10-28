// Пример вывода массива пикселей в окно в реальном времени.
//
// Каждый кадр программа на CPU заполняет буфер RGBA8 цветом, который плавно
// меняется со временем, и выводит его в окно через glDrawPixels. OpenGL здесь
// используется только для того, чтобы показать готовый массив пикселей.
// Раз в секунду в консоль печатается среднее время кадра.

#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "mesh_utils.h"
#include "image_utils.h"
#include "misc.h"
#include "scenes.h"
#include "matrices.h"

int main(int argc, char *argv[])
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    Mesh model1 = load_obj("resources/Bunny.obj");
    if (model1.vert_count <= 0 || model1.tri_count <= 0)
    {
        printf("Couldn't read file: %s", argv[3]);
        return -1;
    }
    mesh_normalize(model1);
    mesh_compute_normals(&model1);

    Mesh model2 = load_obj("resources/Chicken_01.obj");
    if (model2.vert_count <= 0 || model2.tri_count <= 0)
    {
        printf("Couldn't read file: %s", argv[3]);
        free_mesh(model2);
        return -1;
    }
    mesh_normalize(model2);
    mesh_compute_normals(&model2);

    int tw1 = 0, th1 = 0;
    float *t1 = load_image_f32_rgb("resources/textures/porcelain.jpg", &tw1, &th1, 2.2f);
    int tw2 = 0, th2 = 0;
    float *t2 = load_image_f32_rgb("resources/textures/Grass_09-256x256.png", &tw2, &th2, 2.2f);

    // Размер буфера пикселей жёстко связан с размером окна, поэтому менять его нельзя
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(1280, 960, "Software Renderer", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    // 0 - не ждать вертикальной синхронизации: время кадра показывает реальную
    // скорость программы, а не частоту монитора. 1 - включить vsync.
    glfwSwapInterval(0);

    // Число пикселей окна может отличаться от width x height (масштабирование
    // экрана в ОС), поэтому размер буфера берём у самого окна.
    int fb_width = 0, fb_height = 0;
    glfwGetFramebufferSize(window, &fb_width, &fb_height);

    // RGBA8, строки снизу вверх: первая строка буфера - нижняя строка окна
    const size_t pixel_count = (size_t)fb_width * fb_height;
    uint8_t *pixels = malloc(pixel_count << 2);
    float *depth = malloc(pixel_count * sizeof(float));

    float hue_speed = .2f; // оборотов цветового круга в секунду
    float hue = 0.f;

    double prev_time = glfwGetTime();
    double report_time = prev_time;
    int report_frames = 0;

    SceneObj objects[2] = {
        (SceneObj){
            .mesh = &model1,
            .tex = t1, .tex_w = tw1, .tex_h = th1,
            .fallback_albedo = make_v3(.85f, .80f, .72f),
            .instance_count = 3,
            .instances = malloc(3 * sizeof(Mat4x4))
        },
        (SceneObj){
            .mesh = &model2,
            .tex = t2, .tex_w = tw2, .tex_h = th2,
            .fallback_albedo = make_v3(.72f, .78f, .85f),
            .instance_count = 3,
            .instances = malloc(3 * sizeof(Mat4x4))
        }
    };
    Scene scene = {
        .obj_count = 2, .objects = objects
    };

    int max_verts = scene.objects[0].mesh->vert_count;
    for (int i = 1; i < scene.obj_count; i++)
    {
        if (scene.objects[i].mesh->vert_count > max_verts)
            max_verts = scene.objects[i].mesh->vert_count;
    }
    Vec4 *scratch_clip = malloc((size_t)max_verts * sizeof(Vec4));
    Vec3 *scratch_norm = malloc((size_t)max_verts * sizeof(Vec3));

    float fov_y  = (float)M_PI / 3.f;
    float aspect = (float)fb_width / fb_height;
    Vec3 l_world = normalized3(make_v3(.5f, .8f, .4f));

    while (!glfwWindowShouldClose(window))
    {
        double now = glfwGetTime();
        float dt = now - prev_time; // длительность прошлого кадра, с
        prev_time = now;

        memset(pixels, 0, pixel_count << 2);
        for (size_t i = 0; i < pixel_count; i++)
            depth[i] = FLT_MAX;

        for (int k = 0; k < scene.objects[0].instance_count; k++)
        {
            float ox = (k - 1) * .9f;
            scene.objects[0].instances[k] = trs_m4x4(now + k * .7f, Y, make_v3(ox, 0.f, 0.f), .4f);
        }
        if (scene.obj_count > 1)
        {
            for (int k = 0; k < scene.objects[1].instance_count; k++)
            {
                float ox = (k - 1) * .9f;
                scene.objects[1].instances[k] = trs_m4x4(-now + k * .9f, Y, make_v3(ox, 0.f, 1.f), .4f);
            }
        }

        Mat4x4 proj = perspective_m4x4(fov_y, aspect, .1f, 100.f);
        Mat4x4 view = look_at_m4x4(
            make_v3(0.f, 0.f, 4.f),
            make_v3(0.f, 0.f, 0.f),
            make_v3(0.f, 1.f, 0.f));

        Mat3x3 view3 = {
            view.m[0][0], view.m[0][1], view.m[0][2],
            view.m[1][0], view.m[1][1], view.m[1][2],
            view.m[2][0], view.m[2][1], view.m[2][2],
        };
        Vec3 l_view = normalized3(mul_m3x3_v3(view3, l_world));

        scene_draw(
            pixels, depth, fb_width, fb_height,
            &scene, view, proj, l_view,
            scratch_clip, scratch_norm
        );

        for (size_t i = 0; i < pixel_count; i++)
        {
            pixels[i << 2] = powf(pixels[i << 2] / 255.f, 1.f / 2.2f) * 255.f;
            pixels[(i << 2) + 1] = powf(pixels[(i << 2) + 1] / 255.f, 1.f / 2.2f) * 255.f;
            pixels[(i << 2) + 2] = powf(pixels[(i << 2) + 2] / 255.f, 1.f / 2.2f) * 255.f;
        }

        glDrawPixels(fb_width, fb_height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glfwSwapBuffers(window);
        glfwPollEvents();

        report_frames++;
        const double elapsed = glfwGetTime() - report_time;
        if (elapsed >= 1.)
        {
            printf("frame time: %.3f ms (%.1f FPS)\n",
                   1000. * elapsed / report_frames, report_frames / elapsed);
            fflush(stdout);
            report_time += elapsed;
            report_frames = 0;
        }
    }

    free(pixels);
    glfwDestroyWindow(window);
    glfwTerminate();
    for (int i = 0; i < scene.obj_count; i++)
        free(scene.objects[i].instances);
    free(scratch_clip);
    free(scratch_norm);
    free_mesh(model1);
    free_mesh(model2);
    free(t1);
    free(t2);
    return 0;
}
