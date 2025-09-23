#pragma once

#include "platform/window.h"
#include "render/renderer.h"


typedef struct {
    window_info window;             // main window
    renderer_state renderer;        // main renderer
    b8 is_running;
} application_state;


//
b8 application_init(int argc, char *argv[]);

//
void application_shutdown();

//
void application_run();

//
void application_set_fps_values(const u16 target_fps);


// -------------------- GETTER/SETTER --------------------

// get main renderer ptr
renderer_state* application_get_renderer();

// get main window
window_info* application_get_window();
