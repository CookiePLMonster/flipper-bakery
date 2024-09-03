#include "digital_clock_app.hpp"

#include <cookie/enum_class>

void scene_clock_on_enter(void* context) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);

    view_dispatcher_switch_to_view(*app->m_view_dispatcher, FuriEnumParam(AppView::Clock));
}

void scene_clock_on_exit(void* context) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);
    UNUSED(app);
}

bool scene_clock_on_event(void* context, SceneManagerEvent event) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);
    UNUSED(app);
    UNUSED(event);

    return false;
}
