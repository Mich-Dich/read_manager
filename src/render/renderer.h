#pragma once

#include "util/core_config.h"
#include "platform/window.h"

#if defined(RENDER_API_VULKAN)
    typedef struct {
        // TODO: add Vulkan-specific data here later
        b8 initialized;
    } renderer_state;

#elif defined(RENDER_API_OPENGL)
    typedef struct {
        // TODO: add OpenGL-specific data here later
        b8 initialized;
    } renderer_state;

#else
    #error "no render API enabled"
#endif




b8 renderer_init(renderer_state* renderer);


void renderer_shutdown(renderer_state* renderer);


void renderer_begin_frame(renderer_state* renderer);


void renderer_end_frame(window_info* window);


void renderer_on_resize(renderer_state* renderer, u16 width, u16 height);
