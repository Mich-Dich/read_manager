#pragma once

#include <GLFW/glfw3.h>

#include "util/data_structure/data_types.h"



// @brief Structure containing information about a GLFW window.
//        Stores the native GLFW window pointer along with metadata
//        such as width, height, title, and a custom should-close flag.
typedef struct {
    GLFWwindow*     window_ptr;   ///< Pointer to the GLFW window instance.
    u16             width;        ///< Window width in pixels.
    u16             height;       ///< Window height in pixels.
    const char*     title;        ///< Title of the window.
    b8              should_close; ///< Custom flag indicating if the window should close.
} window_info;


// @brief Creates a new GLFW window and initializes its OpenGL context.
//        Also sets up basic window hints, enables VSync, and stores the
//        window parameters into the provided `window_info` structure.
// @param window_data Pointer to a `window_info` struct where window details will be stored.
// @param width       The width of the window in pixels.
// @param height      The height of the window in pixels.
// @param title       The title of the window (displayed in the title bar).
// @return Returns `true` if the window was successfully created, otherwise returns `false`.
b8 create_window(window_info* window_data, const u16 width, const u16 height, const char* title);


// @brief Destroys the specified GLFW window, releases resources, and terminates the GLFW context if applicable.
// @param window_data Pointer to the `window_info` struct representing the window to destroy.
void destroy_window(window_info* window_data);


// @brief Checks whether the window should close. Combines both the custom `should_close` flag and GLFW's internal flag.
// @param window_data Pointer to the `window_info` struct representing the window.
// @return Returns `true` if the window should close, otherwise `false`.
b8 window_should_close(window_info* window_data);


// @brief Polls for and processes all pending window events.
//        This function must be called every frame to keep the application responsive.
void window_poll_events();


// @brief Swaps the front and back buffers of the specified window.
//        Used in double-buffered rendering to display the rendered frame.
// @param window_data Pointer to the `window_info` struct representing the window.
void window_swap_buffers(window_info* window_data);


// @brief Sets the custom `should_close` flag for the specified window.
//        This does not directly call `glfwSetWindowShouldClose`, but instead
//        lets the application manage shutdown logic explicitly.
// @param window_data Pointer to the `window_info` struct representing the window.
// @param value       Boolean value indicating whether the window should close.
void window_set_should_close(window_info* window_data, b8 value);
