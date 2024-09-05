#include "digital_clock_app.hpp"

#include "scenes/scenes.hpp"

#include <cookie/enum_class>
#include <stm32wbxx_ll_rtc.h>

DigitalClockApp::DigitalClockApp()
    : m_scene_manager(scene_handlers, this)
    , m_time_sync_thread(
          "TimeSync",
          1024,
          [](void* context) {
              DigitalClockApp* view = reinterpret_cast<DigitalClockApp*>(context);
              return view->TimeSyncThread();
          },
          this) {
    view_dispatcher_attach_to_gui(*m_view_dispatcher, *m_gui, ViewDispatcherTypeFullscreen);

    view_dispatcher_set_event_callback_context(*m_view_dispatcher, this);
    view_dispatcher_set_custom_event_callback(*m_view_dispatcher, &CustomEventCallback);
    view_dispatcher_set_navigation_event_callback(*m_view_dispatcher, &BackEventCallback);

    view_dispatcher_add_view(
        *m_view_dispatcher, FuriEnumParam(AppView::Init), *m_init_view.View());
    view_dispatcher_add_view(
        *m_view_dispatcher, FuriEnumParam(AppView::Clock), *m_clock_view.View());
}

DigitalClockApp::~DigitalClockApp() {
    view_dispatcher_remove_view(*m_view_dispatcher, FuriEnumParam(AppView::Clock));
    view_dispatcher_remove_view(*m_view_dispatcher, FuriEnumParam(AppView::Init));

    furi_thread_join(*m_time_sync_thread);
}

void DigitalClockApp::Run() {
    scene_manager_next_scene(*m_scene_manager, FuriEnumParam(AppScene::Init));
    view_dispatcher_run(*m_view_dispatcher);
}

void DigitalClockApp::Exit() {
    scene_manager_stop(*m_scene_manager);
    view_dispatcher_stop(*m_view_dispatcher);
}

void DigitalClockApp::SwitchToView(AppView view) const {
    view_dispatcher_switch_to_view(*m_view_dispatcher, FuriEnumParam(view));
}

void DigitalClockApp::SendAppEvent(AppFlowEvent event) const {
    view_dispatcher_send_custom_event(*m_view_dispatcher, FuriEnumParam(event));
}

bool DigitalClockApp::SearchAndSwitchToAnotherScene(AppScene scene) const {
    return scene_manager_search_and_switch_to_another_scene(
        *m_scene_manager, FuriEnumParam(scene));
}

void DigitalClockApp::NextScene(AppScene scene) const {
    scene_manager_next_scene(*m_scene_manager, FuriEnumParam(scene));
}

void DigitalClockApp::StartTimeSync() {
    furi_thread_set_priority(*m_time_sync_thread, FuriThreadPriorityIdle);
    furi_thread_start(*m_time_sync_thread);
}

void DigitalClockApp::StopTimeSync() {
    furi_thread_flags_set(
        furi_thread_get_id(*m_time_sync_thread), FuriEnumParam(SyncThreadEvent::Shutdown));
}

bool DigitalClockApp::CustomEventCallback(void* context, uint32_t event) {
    DigitalClockApp* app = reinterpret_cast<DigitalClockApp*>(context);
    return scene_manager_handle_custom_event(*app->m_scene_manager, event);
}

bool DigitalClockApp::BackEventCallback(void* context) {
    DigitalClockApp* app = reinterpret_cast<DigitalClockApp*>(context);
    return scene_manager_handle_back_event(*app->m_scene_manager);
}

int32_t DigitalClockApp::TimeSyncThread() {
    auto GetSeconds = [] {
        FURI_CRITICAL_ENTER();
        volatile uint32_t seconds = LL_RTC_TIME_GetSecond(RTC);
        FURI_CRITICAL_EXIT();
        return __LL_RTC_CONVERT_BCD2BIN(seconds);
    };

    const uint32_t start_seconds = GetSeconds();
    const uint32_t target_seconds = (start_seconds + 3) % 60;

    // Spin the thread until the next second, then order to switch to the next scene ASAP
    while(true) {
        uint32_t events =
            furi_thread_flags_wait(FuriEnumParam(SyncThreadEvent::Mask), FuriFlagWaitAny, 0);

        if((events & FuriFlagError) == 0 &&
           (events & FuriEnumParam(SyncThreadEvent::Shutdown)) != 0) {
            break;
        }

        const uint32_t seconds = GetSeconds();
        if(seconds == target_seconds) {
            SendAppEvent(AppFlowEvent::SyncDone);

            // Switch away ASAP to let the scene switch
            furi_thread_yield();
            break;
        }

        furi_thread_yield();
    }

    return 0;
}
