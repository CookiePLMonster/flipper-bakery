#pragma once

#include <cookie/gui/gui>
#include <cookie/gui/scene_manager>
#include <cookie/gui/view>
#include <cookie/gui/view_dispatcher>

#include "views/view_clock.hpp"

struct ViewDispatcher;

enum class AppView {
    Clock,
};

class DigitalClockApp {
public:
    DigitalClockApp();
    ~DigitalClockApp();

    void Run();

private:
    static bool CustomEventCallback(void* context, uint32_t event);
    static bool BackEventCallback(void* context);

public:
    cookie::ViewDispatcher m_view_dispatcher;

private:
    cookie::Gui m_gui;

    DigitalClockView m_clock_view;
    cookie::SceneManager m_scene_manager;
};
