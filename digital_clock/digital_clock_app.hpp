#pragma once

#include <cookie/thread>
#include <cookie/within>
#include <cookie/gui/gui>
#include <cookie/gui/scene_manager>
#include <cookie/gui/view>
#include <cookie/gui/view_dispatcher>

#include "views/view_init.hpp"
#include "views/view_clock.hpp"

#include "scenes/scenes.hpp"

enum class AppView {
    Init,
    Clock,
};

enum class AppFlowEvent {
    SyncDone,
};

class DigitalClockApp : cookie::EnableWithin<DigitalClockApp> {
public:
    DigitalClockApp();
    ~DigitalClockApp();

    void Run();
    void Exit();
    void SwitchToView(AppView view) const;
    void SendAppEvent(AppFlowEvent event) const;

    void NextScene(AppScene scene) const;
    bool SearchAndSwitchToAnotherScene(AppScene scene) const;

    void StartTimeSync();
    void StopTimeSync();

private:
    static bool CustomEventCallback(void* context, uint32_t event);
    static bool BackEventCallback(void* context);

    enum class SyncThreadEvent {
        Shutdown = 1 << 0,

        Mask = 1,
    };
    int32_t TimeSyncThread();

private:
    cookie::Gui m_gui;

    cookie::ViewDispatcher m_view_dispatcher;
    InitView m_init_view;
    DigitalClockView m_clock_view;
    cookie::SceneManager m_scene_manager;

    // TODO: If we get RTC alarms in the future, this will become pointless
    // Consider introducing cookie::FuriThread_Nullable for this, and keep the thread alive only for the duration of the scene
    cookie::FuriThread m_time_sync_thread;

public:
    DEFINE_GET_FROM_INNER(m_clock_view);
};
