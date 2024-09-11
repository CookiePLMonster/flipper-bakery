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
    using SplashTask = cookie::SkippableTask<cookie::FuriEventLoopTimer>;

    struct Model {
        bool display_second_line;
    };

    void OnEnter();
    static void OnDraw(Canvas* canvas, const Model& model);

    SplashTask ProcessSplashAsync(::FuriEventLoop* event_loop);

    outer_type* get_outer() const;

private:
    cookie::ViewModel<Model> m_view;
    SplashTask m_splash_task;
};
