#include "digital_clock_app.hpp"

#include <cookie/enum_class>

void scene_init_on_enter(void* context) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);
    app->SwitchToView(AppView::Init);
    app->StartTimeSync();
}

void scene_init_on_exit(void* context) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);
    app->StopTimeSync();
}

bool scene_init_on_event(void* context, SceneManagerEvent event) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);
    if(event.type == SceneManagerEventTypeCustom &&
       event.event == FuriEnumParam(AppFlowEvent::SyncDone)) {
        return app->SearchAndSwitchToAnotherScene(AppScene::Clock);
    }

    return false;
}
