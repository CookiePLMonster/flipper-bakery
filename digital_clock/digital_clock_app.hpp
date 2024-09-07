#pragma once

#include <cookie/semaphore>
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

enum class AppLogicEvent {
    GoToNextScene,
    TickTock,
};

class DigitalClockApp : cookie::EnableWithin<DigitalClockApp> {
public:
    DigitalClockApp();
    ~DigitalClockApp();

    void Run();
    void Exit();
    void SwitchToView(AppView view) const;
    void SendAppEvent(AppLogicEvent event) const;

    void NextScene(AppScene scene) const;
    bool SearchAndSwitchToAnotherScene(AppScene scene) const;

    void StartTickTockTimer();
    void StopTickTockTimer();

    ::FuriEventLoop* GetEventLoop() const;

private:
    static bool CustomEventCallback(void* context, uint32_t event);
    static bool BackEventCallback(void* context);

private:
    cookie::Gui m_gui;

    cookie::ViewDispatcher m_view_dispatcher{m_gui};
    InitView m_init_view;
    DigitalClockView m_clock_view;
    cookie::SceneManager m_scene_manager{scene_handlers, this};

    // We can't submit a custom view dispatcher event from the ISR, as that tries to block.
    // Work it around by releasing a semaphore from the ISR, and putting a custom event in the queue this way instead.
    // Can be removed once view_dispatcher_send_custom_event works from the ISR.
    cookie::FuriBinarySemaphore m_tick_tock_semaphore;
    bool m_tick_tock_timer_running = false;

public:
    DEFINE_GET_FROM_INNER(m_init_view);
    DEFINE_GET_FROM_INNER(m_clock_view);
};
