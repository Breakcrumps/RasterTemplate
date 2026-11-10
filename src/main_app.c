// Пример вывода массива пикселей в окно в реальном времени.
//
// Каждый кадр программа на CPU заполняет буфер RGBA8 цветом, который плавно
// меняется со временем, и выводит его в окно через glDrawPixels. OpenGL здесь
// используется только для того, чтобы показать готовый массив пикселей.
// Раз в секунду в консоль печатается среднее время кадра.

#define CLAMP(a, min, max) (a >= max ? max : a <= min ? min : a)

#include <GLFW/glfw3.h>

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <interpolation.h>
#include "mesh_utils.h"
#include "scenes.h"
#include "matrices.h"
#include "uniforms.h"
#include "glfw_utils.h"
#include "texture.h"
#include "skybox.h"
#include "terrain.h"

static bool just_pressed(GLFWwindow *w, int key)
{
    static char prev[GLFW_KEY_LAST + 1] = {0};
    bool cur = glfwGetKey(w, key) == GLFW_PRESS;
    bool res = cur && !prev[key];
    prev[key] = cur;
    return res;
}

int main(int argc, char *argv[])
{
    if (argc < 5)
        return -1;
    
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    
    Mesh model1 = load_obj("resources/Bunny.obj");
    if (model1.vert_count <= 0 || model1.tri_count <= 0)
        return -1;
    mesh_normalize(model1);
    mesh_compute_normals(&model1);
    
    Mesh model2 = load_obj("resources/Chicken_01.obj");
    if (model2.vert_count <= 0 || model2.tri_count <= 0)
    {
        mesh_free(model1);
        return -1;
    }
    mesh_normalize(model2);
    mesh_compute_normals(&model2);

    Mesh model3 = load_obj("resources/Roman Centurion.obj");
    if (model3.vert_count <= 0 || model3.tri_count <= 0)
    {
        mesh_free(model1);
        mesh_free(model2);
        return -1;
    }
    mesh_normalize(model3);
    mesh_compute_normals(&model3);
    
    Texture tex_porc = texture_load("resources/textures/porcelain.jpg", 2.2f);
    Texture tex_grass = texture_load("resources/textures/Grass_09-256x256.png", 2.2f);
    
    // Размер буфера пикселей жёстко связан с размером окна, поэтому менять его нельзя
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    int fb_width = atoi(argv[1]), fb_height = atoi(argv[2]);
    size_t out_pixel_count = (size_t)fb_width * fb_height;
    uint8_t *out_pixels = malloc(out_pixel_count << 2);
    GLFWwindow *window = glfwCreateWindow(fb_width, fb_height, "Software Renderer", NULL, NULL);
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
    
    // // Число пикселей окна может отличаться от width x height (масштабирование
    // // экрана в ОС), поэтому размер буфера берём у самого окна.
    // int fb_width = 0, fb_height = 0;
    // glfwGetFramebufferSize(window, &fb_width, &fb_height);

    int render_width = atoi(argv[3]), render_height = atoi(argv[4]);
    
    // RGBA8, строки снизу вверх: первая строка буфера - нижняя строка окна
    const size_t pixel_count = (size_t)render_width * render_height;
    uint8_t *pixels = malloc(pixel_count << 2);
    float *depth = malloc(pixel_count * sizeof(float));
    
    double prev_time = glfwGetTime();
    double report_time = prev_time;
    int report_frames = 0;
    
    SceneObj objects[3] = {
        (SceneObj){
            .mesh = &model1,
            .tex = tex_porc,
            .fallback_albedo = make_v3(.85f, .80f, .72f),
            .instance_count = 1,
            .instances = malloc(sizeof(Mat4x4))
        },
        (SceneObj){
            .mesh = &model2,
            .tex = {0},
            .fallback_albedo = make_v3(.72f, .78f, .85f),
            .instance_count = 2,
            .instances = malloc(2 * sizeof(Mat4x4))
        },
        (SceneObj){
            .mesh = &model3,
            .tex = {0},
            .fallback_albedo = make_v3(1.f, 1.f, 1.f),
            .instance_count = 1,
            .instances = malloc(sizeof(Mat4x4))
        }
    };
    Scene scene = {
        .obj_count = 3, .objects = objects
    };


    Cubemap cubemap = cubemap_load((char *[6]){
        "resources/textures/skybox/hills_lf.png", "resources/textures/skybox/hills_rt.png",
        "resources/textures/skybox/hills_up.jpg", "resources/textures/skybox/hills_dn.png",
        "resources/textures/skybox/hills_bk.png", "resources/textures/skybox/hills_ft.png"
    });

    Texture tex_rock = texture_load("resources/textures/Grass_13-256x256.png", 2.2f);

    Terrain terrain;
    terrain_init(&terrain, 24, 5.f, 4.f, 1337u, 5);

    static const struct { float dx_step, z, y_bonus, scale, a_step; } layout[3] = {
        { .9f, 0.f, .1f, .4f, .7f },
        { .9f, 1.f, .1f, .4f, .9f },
        { .9f, 2.f, .5f, 1.f, .9f },
    };
    for (int o = 0; o < 3; o++)
    {
        for (int k = 0; k < scene.objects[o].instance_count; k++)
        {
            float ox = (k - 1) * layout[o].dx_step;
            float oy = terrain_surface_height(&terrain, ox, layout[o].z) + layout[o].y_bonus;
            scene.objects[o].instances[k] = trs_m4x4(
                k * layout[o].a_step, Y,
                make_v3(ox, oy, layout[o].z),
                layout[o].scale
            );
        }
    }
    
    int max_verts = scene.objects[0].mesh->vert_count;
    for (int i = 1; i < scene.obj_count; i++)
    {
        if (scene.objects[i].mesh->vert_count > max_verts)
            max_verts = scene.objects[i].mesh->vert_count;
    }
    Vec4 *scratch_clip = malloc(max_verts * sizeof(Vec4));
    Vec3 *scratch_norm = malloc(max_verts * sizeof(Vec3));
    
    float fov_y  = (float)M_PI / 3.f;
    float aspect = (float)fb_width / fb_height;
    Vec3 l_world = normalized3(make_v3(-.5f, .8f, .4f));    

    float yaw = 0.f, pitch = 0.f;
    Vec3 camera_pos = { 1.f, 1.f, 2.f };
    float speed = 5.f, sens = .0025f;
    bool firstFrame = true;
    double last_mx = 0., last_my = 0.;

    bool ray_skybox = true;
    ShadeMode shade_mode = 0;
    UpscaleMode upscale_mode = UPSCALE_NONE;
    bool ground_cam = false;
    float eye_height = 1.5f;

    if (upscale_mode == UPSCALE_WHOLE)
    {
        int k = fb_width / render_width;
        if (k * render_width != fb_width || k * render_height != fb_height)
            upscale_mode = UPSCALE_BILINEAR;
    }
    if (upscale_mode == UPSCALE_NONE)
    {
        if (render_width != fb_width || render_height != fb_height)
            upscale_mode = UPSCALE_BILINEAR;
    }
    
    while (!glfwWindowShouldClose(window))
    {
        double now = glfwGetTime();
        float dt = now - prev_time; // длительность прошлого кадра, с
        prev_time = now;
        
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        if (firstFrame)
            last_mx = mx, last_my = my, firstFrame = false;
        float d_mx = mx - last_mx, d_my = my - last_my;
        last_mx = mx, last_my = my;
        
        yaw += d_mx * sens;
        pitch -= d_my * sens;        
        pitch = CLAMP(pitch, -1.55f, 1.55f);
        
        Vec3 forward = normalized3(make_v3(
            sinf(yaw) * cosf(pitch),
            sinf(pitch),
            -cosf(yaw) * cosf(pitch)
        ));
        Vec3 worldUp = make_v3(0.f, 1.f, 0.f);
        Vec3 right = normalized3(cross3(forward, worldUp));
        
        Vec3 moveDir = make_v3(0.f, 0.f, 0.f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            moveDir = add3(moveDir, forward);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            moveDir = sub3(moveDir, forward);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            moveDir = add3(moveDir, right);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            moveDir = sub3(moveDir, right);
        
        camera_pos = add3(camera_pos, scaled3(normalized3(moveDir), speed * dt));

        if (ground_cam)
            camera_pos.y = terrain_surface_height(&terrain, camera_pos.x, camera_pos.z) + eye_height;
        
        Mat4x4 view = look_at_m4x4(
            camera_pos,
            add3(camera_pos, forward),
            cross3(right, forward)
        );
        Mat3x3 view3 = {
            view.m[0][0], view.m[0][1], view.m[0][2],
            view.m[1][0], view.m[1][1], view.m[1][2],
            view.m[2][0], view.m[2][1], view.m[2][2],
        };

        if (just_pressed(window, GLFW_KEY_F1))
        {
            shade_mode ^= SHADE_LOD;
            printf("LOD mode: %s\n", shade_mode & SHADE_LOD ? "On" : "Off");
        }
        if (just_pressed(window, GLFW_KEY_F2))
        {
            shade_mode ^= SHADE_DERIVATIVE;
            printf("Derivative mode: %s\n", shade_mode & SHADE_DERIVATIVE ? "On" : "Off");
        }
        if (just_pressed(window, GLFW_KEY_F3))
        {
            shade_mode ^= SHADE_DEPTH_ONLY;
            printf("Depth lighting mode: %s\n", shade_mode & SHADE_DEPTH_ONLY ? "On" : "Off");
        }
        if (just_pressed(window, GLFW_KEY_F4))
        {
            shade_mode ^= SHADE_SOLID;
            printf("No texture mode: %s\n", shade_mode & SHADE_SOLID ? "On" : "Off");
        }
        if (just_pressed(window, GLFW_KEY_R))
        {
            ray_skybox = !ray_skybox;
            printf("Ray skybox mode: %s\n", ray_skybox ? "On" : "Off");
        }
        if (just_pressed(window, GLFW_KEY_G))
        {
            ground_cam = !ground_cam;
            printf("LOD mode: %s\n", shade_mode & SHADE_LOD ? "On" : "Off");
        }
        
        const float near = .1f, far = 100.f;
        
        Uniforms uni = {
            .proj = perspective_m4x4(fov_y, aspect, near, far),
            .view = view,
            .l_clip = normalized3(mul_m3x3_v3(view3, l_world)),
            .near = near, .far = far,
            .cull_backfaces = true,
            .tex_scale = 1.f, .lod_bias = 0.f,
            .shade_mode = shade_mode
        };
        
        uni.vp = mul4x4(uni.proj, uni.view);
        
        memset(pixels, 0, pixel_count << 2);
        for (size_t i = 0; i < pixel_count; i++)
        {
            depth[i] = FLT_MAX;
        }
        
        scene_draw(
            pixels, depth, render_width, render_height,
            &scene, &uni,
            scratch_clip, scratch_norm
        );
        terrain_draw(pixels, depth, render_width, render_height, &terrain, camera_pos, &uni, &tex_grass, &tex_rock);
        if (ray_skybox)
            draw_skybox_ray(pixels, depth, render_width, render_height, &cubemap, &uni);
        else
            draw_skybox_geom(pixels, depth, render_width, render_height, &cubemap, &uni, camera_pos);

        if (upscale_mode == UPSCALE_NEAREST)
            nearest(out_pixels, fb_width, fb_height, pixels, render_width, render_height);
        else if (upscale_mode == UPSCALE_WHOLE)
            whole(out_pixels, fb_width, pixels, render_width, render_height);
        else if (upscale_mode == UPSCALE_BILINEAR)
            bilinear(out_pixels, fb_width, fb_height, pixels, render_width, render_height);
        else
            memcpy(out_pixels, pixels, out_pixel_count << 2);

        gamma_correct(out_pixels, out_pixel_count);
        
        glDrawPixels(fb_width, fb_height, GL_RGBA, GL_UNSIGNED_BYTE, out_pixels);
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        report_frames++;
        const double elapsed = glfwGetTime() - report_time;
        if (elapsed >= 1.)
        {
            printf("frame time: %.3f ms (%.1f FPS)\n",
                1000. * elapsed / report_frames, report_frames / elapsed
            );
            fflush(stdout);
            report_time += elapsed;
            report_frames = 0;
        }
    }
    
    free(pixels);
    free(out_pixels);
    glfwDestroyWindow(window);
    glfwTerminate();
    for (int i = 0; i < scene.obj_count; i++)
    {
        free(scene.objects[i].instances);
    }
    free(scratch_clip);
    free(scratch_norm);
    mesh_free(model1);
    mesh_free(model2);
    mesh_free(model3);
    texture_free(&tex_porc);
    texture_free(&tex_grass);
    texture_free(&tex_rock);
    cubemap_free(&cubemap);
    terrain_free(&terrain);
    return 0;
}
    