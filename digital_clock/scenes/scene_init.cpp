#include "digital_clock_app.hpp"

#include <cookie/common>

void scene_init_on_enter(void* context) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);
    app->SwitchToView(AppView::Init);
    app->StartTimeSync();
}

void scene_init_on_exit(void* context) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);
    UNUSED(app);
}

bool scene_init_on_event(void* context, SceneManagerEvent event) {
    DigitalClockApp* app = static_cast<DigitalClockApp*>(context);
    if(event.type == SceneManagerEventTypeCustom &&
       event.event == cookie::FuriEnumParam(AppFlowEvent::SyncDone)) {
        return app->SearchAndSwitchToAnotherScene(AppScene::Clock);
    }

    return false;
}
