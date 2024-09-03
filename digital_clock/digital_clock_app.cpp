#include "digital_clock_app.hpp"

#include "scenes/scenes.hpp"

#include <cookie/enum_class>

DigitalClockApp::DigitalClockApp()
    : m_scene_manager(scene_handlers, this) {
    view_dispatcher_attach_to_gui(*m_view_dispatcher, *m_gui, ViewDispatcherTypeFullscreen);

    view_dispatcher_set_event_callback_context(*m_view_dispatcher, this);
    view_dispatcher_set_custom_event_callback(*m_view_dispatcher, &CustomEventCallback);
    view_dispatcher_set_navigation_event_callback(*m_view_dispatcher, &BackEventCallback);

    view_dispatcher_add_view(*m_view_dispatcher, FuriEnumParam(AppView::Clock), m_clock_view.View());
}

DigitalClockApp::~DigitalClockApp() {
    view_dispatcher_remove_view(*m_view_dispatcher, FuriEnumParam(AppView::Clock));
}

void DigitalClockApp::Run() {
    scene_manager_next_scene(*m_scene_manager, FuriEnumParam(AppScene::Clock));
    view_dispatcher_run(*m_view_dispatcher);
}

bool DigitalClockApp::CustomEventCallback(void* context, uint32_t event) {
    DigitalClockApp* app = reinterpret_cast<DigitalClockApp*>(context);
    return scene_manager_handle_custom_event(*app->m_scene_manager, event);
}

bool DigitalClockApp::BackEventCallback(void* context) {
    DigitalClockApp* app = reinterpret_cast<DigitalClockApp*>(context);
    return scene_manager_handle_back_event(*app->m_scene_manager);
}
