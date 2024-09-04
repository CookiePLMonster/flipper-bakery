#pragma once

#include <cookie/within>
#include <cookie/gui/gui>
#include <cookie/gui/scene_manager>
#include <cookie/gui/view>
#include <cookie/gui/view_dispatcher>

#include "views/view_clock.hpp"

enum class AppView {
    Clock,
};

class DigitalClockApp : cookie::EnableWithin<DigitalClockApp> {
public:
    DigitalClockApp();
    ~DigitalClockApp();

    void Run();
    void SwitchToView(AppView view) const;

    DEFINE_GET_FROM_INNER(m_clock_view);

private:
    static bool CustomEventCallback(void* context, uint32_t event);
    static bool BackEventCallback(void* context);

private:
    cookie::Gui m_gui;

    cookie::ViewDispatcher m_view_dispatcher;
    DigitalClockView m_clock_view;
    cookie::SceneManager m_scene_manager;
};
