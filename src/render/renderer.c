
#include "util/io/logger.h"
#include "util/core_config.h"
#include "platform/window.h"
#include "imgui_config/imgui_config.h"

#include "renderer.h"


#if defined(RENDER_API_VULKAN)
    // Not to be implemented for a long while

#elif defined(RENDER_API_OPENGL)

    #include <GL/gl.h>
    #include <GL/glx.h>
    #include <GLFW/glfw3.h>


    b8 renderer_init(__attribute_maybe_unused__ renderer_state* renderer) {
        
        // Get OpenGL version info
        LOG(Trace, "OpenGL version: %s", glGetString(GL_VERSION));
        LOG(Trace, "OpenGL vendor: %s", glGetString(GL_VENDOR));
        LOG(Trace, "OpenGL renderer: %s", glGetString(GL_RENDERER));

        // renderer->initialized = true;
        // LOG_INIT
        return true;
    }

    void renderer_shutdown(renderer_state* renderer) {
        
        // imgui_shutdown();
        renderer->initialized = false;
        LOG_SHUTDOWN
    }

// ============================================================================================================================================
// draw
// ============================================================================================================================================

    void renderer_begin_frame(renderer_state* renderer) {
        
        if (!renderer->initialized) return;
        imgui_begin_frame();

        // ImGui demo code
        if (igBegin("My Window", NULL, 0)) {
            igText("Hello, world!");
            igEnd();
        }
    }

    void renderer_end_frame(window_info* window) {
        
        imgui_end_frame(window);
        window_swap_buffers(window);
    }

    void renderer_on_resize(renderer_state* renderer, u16 width, u16 height) {
        
        if (!renderer->initialized) return;
        glViewport(0, 0, width, height);
    }

#endif
