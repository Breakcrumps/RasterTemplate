#ifndef GLFW_UTILS_H
#define GLFW_UTILS_H

#include <GLFW/glfw3.h>
#include <stdio.h>

static inline void glfw_error_callback(int error, const char *description)
{
    fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

static inline void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

#endif
