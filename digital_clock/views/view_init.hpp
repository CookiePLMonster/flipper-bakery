#pragma once

#include <cookie/coroutine>
#include <cookie/event_loop>
#include <cookie/within>
#include <cookie/gui/view>

class DigitalClockApp;

class InitView : cookie::Within<InitView, DigitalClockApp> {
public:
    InitView();

    const auto& View() const {
        return m_view;
    }

private:
    struct Model {
        bool display_second_line;
    };

    void OnEnter();
    void OnExit();
    static void OnDraw(Canvas* canvas, const Model& model);

    cookie::Task<> ProcessSplashAsync();

    outer_type* get_outer() const;

private:
    cookie::ViewModel<Model> m_view;
    cookie::AwaitableTimer<cookie::FuriEventLoopTimer> m_splash_timer;
    cookie::Task<> m_splash_task;
};
