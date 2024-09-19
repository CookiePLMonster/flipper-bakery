#include "digital_clock_app.hpp"

void scene_clock_on_enter(void* context) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);
    app->SwitchToView(AppView::Clock);
}

void scene_clock_on_exit(void* context) {
    UNUSED(context);
}

bool scene_clock_on_event(void* context, SceneManagerEvent event) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);
    if(event.type == SceneManagerEventTypeBack) {
        app->Exit();
        return true;
    }

    return false;
}
