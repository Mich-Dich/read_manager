
#include "util/io/logger.h"
#include "util/io/serializer_yaml.h"
#include "util/crash_handler.h"
#include "util/system.h"
#include "imgui_config/imgui_config.h"
#include "dashboard/dashboard.h"

#include "application.h"


// DEV-ONLY
#include <string.h>
#include <threads.h>
#include <stdatomic.h>
// DEV-ONLY


static application_state app_state;


// ============================================================================================================================================
// getter/setter
// ============================================================================================================================================

renderer_state* application_get_renderer()          { return &app_state.renderer; }

window_info* application_get_window()               { return &app_state.window; }


// ============================================================================================================================================
// long client init
// ============================================================================================================================================

static atomic_bool      init_complete = false;       // flag to track initialization status

// Thread function for initialization
int init_thread(__attribute_maybe_unused__ void* arg) {

    LOGGER_REGISTER_THREAD_LABEL("client init")
    dashboard_init();
    init_complete = true;
    logger_remove_thread_label_by_id((u64)pthread_self());
    return 0;
}


// ============================================================================================================================================
// FPS control
// ============================================================================================================================================

static u32 dashboard_crash_callback;
static f64 s_desired_loop_duration_s = 10.f;       // in seconds
static f64 s_loop_start_time = 0.f;
static f64 s_delta_time = 0.f;

static b8 long_init = false;

//
void application_set_fps_values(const u16 desired_framerate) {

    s_desired_loop_duration_s = 1/(f32)desired_framerate;
    s_loop_start_time = get_precise_time();
}

//
void limit_fps() {

    const f64 current = get_precise_time();                     // I guess time in milliseconds
    const f64 difference = current - s_loop_start_time;
    s_delta_time = s_desired_loop_duration_s - difference;
    if (s_delta_time > 0.f)
        precise_sleep(s_delta_time);
    
    s_loop_start_time = get_precise_time();                     // as this is the last function call before next loop iteration
}

// ============================================================================================================================================
// application
// ============================================================================================================================================


b8 application_init(__attribute_maybe_unused__ int argc, __attribute_maybe_unused__ char *argv[]) {

    char display_name[PATH_MAX] = {0};
    strcpy(display_name, "Application Template");      // set default string incase title cant be loaded from app_settings

    do {        // load app settings
    
        const char* exec_path = get_executable_path();

        char loc_file_path[PATH_MAX];
        memset(loc_file_path, '\0', sizeof(loc_file_path));
        const int written = snprintf(loc_file_path, sizeof(loc_file_path), "%s/%s", exec_path, "config");
        VALIDATE(written >= 0 && (size_t)written < sizeof(loc_file_path), return false, "", "Path too long: %s/%s\n", exec_path, "config");

        SY sy = {0};
        VALIDATE(sy_init(&sy, loc_file_path, "app_settings.yml", "general_settings", SERIALIZER_OPTION_LOAD), break, "", "Failed to load app settings");
        sy_entry_str(&sy, "display_name", display_name, sizeof(display_name));
        sy_entry(&sy, "long_startup_process", &long_init, "%d");
        sy_shutdown(&sy);
        
    } while (0);

    ASSERT(create_window(&app_state.window, 800, 600, display_name), "", "Failed to create window")
    // ASSERT(renderer_init(&app_state.renderer), "", "Failed to initialize renderer")
    imgui_init(&app_state.window);

    dashboard_crash_callback = crash_handler_subscribe_callback(dashboard_on_crash);
    app_state.is_running = true;
    LOG_INIT
    return true;
}


void application_shutdown() {

    crash_handler_unsubscribe_callback(dashboard_crash_callback);

    imgui_shutdown();
    // renderer_shutdown(&app_state.renderer);
    destroy_window(&app_state.window);
    
    LOG_SHUTDOWN
}


void application_run() {

    application_set_fps_values(30);         // set target FPS to 30
    if (long_init) {

        // In your main function or where you want to start the thread:
        thrd_t init_thread_handle;
        const int result = thrd_create(&init_thread_handle, init_thread, NULL);
        VALIDATE(result == thrd_success, return, "", "Failed to create initialization thread [%s]", strerror(result));
        thrd_detach(init_thread_handle);        // Detach the thread so it cleans up automatically when done

        // Main loop
        while (!init_complete) {                    // separate loop without the update and real draw
            window_poll_events();
            imgui_begin_frame();
            dashboard_draw_init_UI(s_delta_time);
            imgui_end_frame(&app_state.window);
            limit_fps();
        }

    } else {

        dashboard_init();
    }
    
    while (!window_should_close(&app_state.window) && app_state.is_running) {
        window_poll_events();
        
        dashboard_update(s_delta_time);
        
        // renderer_begin_frame(&app_state.renderer);
        imgui_begin_frame();
        dashboard_draw(s_delta_time);        
        imgui_end_frame(&app_state.window);
        // renderer_end_frame(&app_state.window);
        
        limit_fps();                        // sets [s_delta_time]
    }
    
    dashboard_shutdown();
}


// ============================================================================================================================================
// util
// ============================================================================================================================================
