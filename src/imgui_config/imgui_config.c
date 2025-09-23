
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <limits.h>

#include "cimgui.h"
#include "cimgui_impl.h"

#include "util/io/logger.h"
#include "util/core_config.h"
#include "util/data_structure/data_types.h"
#include "util/data_structure/unordered_map.h"
#include "util/system.h"
#include "platform/window.h"

#include "imgui_config.h"



static ImVec4 s_clear_color;

static unordered_map s_font_map;
f32 g_font_size = 15.f;
f32 g_big_font_size = 18.f;
f32 g_font_size_header_0 = 19.f;
f32 g_font_size_header_1 = 23.f;
f32 g_font_size_header_2 = 27.f;
f32 g_font_size_giant = 60.f;
static ImGuiContext* s_context_imgui = NULL;


ImFont* imgui_config_get_font(const font_type type) {
    
    void* value = NULL;
    if (u_map_find(&s_font_map, (void*)(uintptr_t)type, &value) == AT_SUCCESS)
        return value;

    return NULL;
}

char* format_path(const char* format, const char* path) {
    static char buffer[512];
    snprintf(buffer, sizeof(buffer), format, path);
    return buffer;
}



void load_fonts() {

    igSetCurrentContext(s_context_imgui);
    // TODO: for ImPlot: implotSetCurrentContext(m_context_implot);
    
    ImGuiIO* io = igGetIO();
    io->FontAllowUserScaling = true;
    
    // Clear font atlas
    ImFontAtlas* atlas = io->Fonts;
    ImFontAtlas_Clear(atlas);
    
    // Clear our font map
    u_map_free(&s_font_map);                                // if not init it will just return a AT_NOT_INITIALIZED
    u_map_init(&s_font_map, 16, ptr_hash, ptr_compare);
    
    // Get base path (implementation specific - you'll need to implement this)
    const char* base_path = get_executable_path();
    char open_sans_path[PATH_MAX];
    snprintf(open_sans_path, sizeof(open_sans_path), "%s/assets/fonts/Open_Sans/static", base_path);
    char inconsolata_path[PATH_MAX];
    snprintf(inconsolata_path, sizeof(inconsolata_path), "%s/assets/fonts/Inconsolata/static", base_path);
        
    ImFont* font;
    
    // Load fonts and store in map
    #define LOAD_FONT(path, font_path, size, type)                                                          \
        font = ImFontAtlas_AddFontFromFileTTF(atlas, format_path(path, font_path), size, NULL, NULL);       \
        u_map_insert(&s_font_map, (void*)(uintptr_t)type, font);

    // Regular fonts
    LOAD_FONT("%s/OpenSans-Regular.ttf", open_sans_path, g_font_size, FT_REGULAR)
    LOAD_FONT("%s/OpenSans-Bold.ttf", open_sans_path, g_font_size, FT_BOLD);
    LOAD_FONT("%s/OpenSans-Italic.ttf", open_sans_path, g_font_size, FT_ITALIC);

    // Big fonts
    LOAD_FONT("%s/OpenSans-Regular.ttf", open_sans_path, g_big_font_size, FT_REGULAR_BIG);
    LOAD_FONT("%s/OpenSans-Bold.ttf", open_sans_path, g_big_font_size, FT_BOLD_BIG);
    LOAD_FONT("%s/OpenSans-Italic.ttf", open_sans_path, g_big_font_size, FT_ITALIC_BIG);
    
    // Header fonts
    LOAD_FONT("%s/OpenSans-Regular.ttf", open_sans_path, g_font_size_header_2, FT_HEADER_0);
    LOAD_FONT("%s/OpenSans-Regular.ttf", open_sans_path, g_font_size_header_1, FT_HEADER_1);
    LOAD_FONT("%s/OpenSans-Regular.ttf", open_sans_path, g_font_size_header_0, FT_HEADER_2);
    LOAD_FONT("%s/OpenSans-Bold.ttf", open_sans_path, g_font_size_giant, FT_GIANT);
    
    // Monospace fonts
    LOAD_FONT("%s/Inconsolata-Regular.ttf", inconsolata_path, g_font_size, FT_MONOSPACE_REGULAR);
    LOAD_FONT("%s/Inconsolata-Regular.ttf", inconsolata_path, g_big_font_size, FT_MONOSPACE_REGULAR_BIG);
    
#undef LOAD_FONT

    // Set default font
    void* default_font;
    if (u_map_find(&s_font_map, (void*)(uintptr_t)FT_REGULAR, &default_font) == AT_SUCCESS) {
        io->FontDefault = (ImFont*)default_font;
    }
    
}


b8 imgui_init(window_info* window_data) {

    VALIDATE(!s_context_imgui, return false, "", "ImGui Context already created")

    s_context_imgui = igCreateContext(NULL);        // setup imgui
    
    // set docking
    ImGuiIO *ioptr = igGetIO();
    ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    //ioptr->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
#ifdef IMGUI_HAS_DOCK
    ioptr->ConfigFlags |= ImGuiConfigFlags_DockingEnable;       // Enable Docking
    ioptr->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;     // Enable Multi-Viewport / Platform Windows
#endif

    // Setup scaling
    ImGuiStyle* style = igGetStyle();
    const float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor()); // Valid on GLFW 3.3+ only
    ImGuiStyle_ScaleAllSizes(style, main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style->FontScaleDpi = main_scale;                   // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 3
    ioptr->ConfigDpiScaleFonts = true;                  // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    ioptr->ConfigDpiScaleViewports = true;              // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.
#endif
    ImGui_ImplGlfw_InitForOpenGL(window_data->window_ptr, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    igStyleColorsDark(NULL);

    load_fonts();

    s_clear_color.x = 0.45f;
    s_clear_color.y = 0.55f;
    s_clear_color.z = 0.60f;
    s_clear_color.w = 1.00f;
    return true;
}


void imgui_shutdown() {

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    igDestroyContext(s_context_imgui);
}


void imgui_begin_frame() {

    ASSERT(s_context_imgui, "", "Tried to render befor creating the imgui context")
    igSetCurrentContext(s_context_imgui);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    igNewFrame();
}


void imgui_end_frame(window_info* window_data) {

    // render
    igRender();
    ImGuiIO *ioptr = igGetIO();
    glfwMakeContextCurrent(window_data->window_ptr);
    glViewport(0, 0, (int)ioptr->DisplaySize.x, (int)ioptr->DisplaySize.y);
    glClearColor(s_clear_color.x, s_clear_color.y, s_clear_color.z, s_clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(igGetDrawData());
#ifdef IMGUI_HAS_DOCK
    if (ioptr->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) 
    {
    GLFWwindow *backup_current_window = glfwGetCurrentContext();
    igUpdatePlatformWindows();
    igRenderPlatformWindowsDefault(NULL, NULL);
    glfwMakeContextCurrent(backup_current_window);
    }
#endif

    glfwSwapBuffers(window_data->window_ptr);
}


ImVec4* imgui_config_get_clear_color_ptr() { return &s_clear_color; }


