
#include "util/io/logger.h"
#include "imgui_config/imgui_config.h"

#include "dashboard.h"


static bool showDemoWindow = true;
static bool showAnotherWindow = false;

//
b8 dashboard_init() {

    return true;
}

//
void dashboard_shutdown() {

    LOG_SHUTDOWN
}

//
void dashboard_on_crash() { LOG(Debug, "User crash_callback")}


//
void dashboard_update(__attribute_maybe_unused__ const f32 delta_time) { }

//
void dashboard_draw(__attribute_maybe_unused__ const f32 delta_time) {

    if (showDemoWindow)
        igShowDemoWindow(&showDemoWindow);

    // show a simple window that we created ourselves.
    {
        static float f = 0.0f;
        static int counter = 0;

        igBegin("Hello, world!", NULL, 0);
        igText("This is some useful text");
        igCheckbox("Demo window", &showDemoWindow);
        igCheckbox("Another window", &showAnotherWindow);

        igSliderFloat("Float", &f, 0.0f, 1.0f, "%.3f", 0);
        igColorEdit3("clear color", (float *)imgui_config_get_clear_color_ptr(), 0);

        ImVec2 buttonSize;
        buttonSize.x = 0;
        buttonSize.y = 0;
        if (igButton("Button", buttonSize))
            counter++;
        igSameLine(0.0f, -1.0f);
        igText("counter = %d", counter);

        igText("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / igGetIO()->Framerate, igGetIO()->Framerate);
        igEnd();
    }

    if (showAnotherWindow) {
        igBegin("imgui Another Window", &showAnotherWindow, 0);
        igText("Hello from imgui");
        ImVec2 buttonSize;
        buttonSize.x = 0;
        buttonSize.y = 0;
        if (igButton("Close me", buttonSize)) {
            showAnotherWindow = false;
        }
        igEnd();
    }
}


void dashboard_draw_init_UI(const f32 delta_time) {

    // TODO: implement Init UI
}
