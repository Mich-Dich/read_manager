#pragma once

#include <cimgui.h>
#include "cimgui_impl.h"
#include "util/data_structure/data_types.h"
#include "platform/window.h"

// #define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#ifdef IMGUI_HAS_IMSTR
    #define igBegin igBegin_Str
    #define igSliderFloat igSliderFloat_Str
    #define igCheckbox igCheckbox_Str
    #define igColorEdit3 igColorEdit3_Str
    #define igButton igButton_Str
#endif

#define igGetIO igGetIO_Nil

extern f32 g_font_size;
extern f32 g_big_font_size;
extern f32 g_font_size_header_0;
extern f32 g_font_size_header_1;
extern f32 g_font_size_header_2;
extern f32 g_font_size_giant;


typedef enum {
    FT_REGULAR,
    FT_REGULAR_BIG,
    FT_BOLD,
    FT_BOLD_BIG,
    FT_ITALIC,
    FT_ITALIC_BIG,
    FT_HEADER_0,
    FT_HEADER_1,
    FT_HEADER_2,
    FT_GIANT,
    FT_MONOSPACE_REGULAR,
    FT_MONOSPACE_REGULAR_BIG,
} font_type;


// @brief Initializes the ImGui context and sets up platform/renderer bindings.
//        Configures ImGui for use with OpenGL and GLFW, sets up DPI scaling,
//        and initializes default style and font settings.
// @param window GLFW window to attach ImGui to
// @return True if initialization succeeded, false otherwise
b8 imgui_init(window_info* window);


// @brief Cleans up ImGui resources and shuts down platform/renderer bindings.
//        Should be called before application termination to properly release
//        all resources allocated by ImGui.
void imgui_shutdown();


// @brief Starts a new ImGui frame.
//        Prepares ImGui for receiving new UI commands for the current frame.
//        Must be called before any ImGui UI rendering commands.
void imgui_begin_frame();


// @brief Ends the current ImGui frame and renders the UI.
//        Finalizes the current UI frame, renders to the active framebuffer,
//        and handles platform window management (if viewports are enabled).
// @param window_data GLFW window data for rendering and buffer swap
void imgui_end_frame(window_info* window_data);


// @brief Gets a pointer to the clear color used by the ImGui renderer.
//        The clear color is used as the background color when rendering
//        the ImGui viewport. Can be modified to change background color.
// @return Pointer to the ImVec4 clear color (RGBA format)
ImVec4* imgui_config_get_clear_color_ptr();



ImFont* imgui_config_get_font(const font_type type);

