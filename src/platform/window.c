

#include "window.h"

#include "render/renderer.h"
#include "application.h"
#include "util/io/logger.h"


// ============================================================================================================================================
// callbacks
// ============================================================================================================================================


static void glfw_error_callback(int error, const char* description) {
    LOG(Error, "GLFW Error (%d): %s", error, description);
}


// static void glfw_framebuffer_size_callback(__attribute_maybe_unused__ GLFWwindow* window, int width, int height) {
//     renderer_state* renderer = application_get_renderer();         // get main renderer ptr
//     renderer_on_resize(renderer, width, height);
// }


// ============================================================================================================================================
// window
// ============================================================================================================================================


b8 create_window(window_info* window_data, const u16 width, const u16 height, const char* title) {

    ASSERT(glfwInit(), "", "Failed to init GLFW")
    glfwSetErrorCallback(glfw_error_callback);

    // Decide GL+GLSL versions
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    // to use scale-factor:            glfwCreateWindow((int)(1280 * main_scale), (int)(800 * main_scale), ...);
    window_data->window_ptr = glfwCreateWindow(width, height, title, NULL, NULL);
    ASSERT(window_data->window_ptr, "", "Failed to create window")
    window_data->width = width;
    window_data->height = height;
    window_data->title = title;
    window_data->should_close = false;

    glfwMakeContextCurrent(window_data->window_ptr);
    // glfwSetFramebufferSizeCallback(window_data->window_ptr, glfw_framebuffer_size_callback);
    glfwSwapInterval(1);                                        // enable vsync
    
    LOG(Trace, "OpenGL version: %s\n", (char *)glGetString(GL_VERSION));
    LOG_INIT
    return true;
}


void destroy_window(window_info* window_data) {

    if (window_data->window_ptr) {
        glfwDestroyWindow(window_data->window_ptr);
        window_data->window_ptr = NULL;
    }
    glfwTerminate();
    LOG(Trace, "shutdown window [%s]", window_data->title)
}


// ============================================================================================================================================
// usage
// ============================================================================================================================================


b8 window_should_close(window_info* window_data)                    { return window_data->should_close || glfwWindowShouldClose(window_data->window_ptr); }


void window_poll_events()                                           { glfwPollEvents(); }


void window_swap_buffers(window_info* window_data)                  { glfwSwapBuffers(window_data->window_ptr); }


void window_set_should_close(window_info* window_data, b8 value)    { window_data->should_close = value; }

