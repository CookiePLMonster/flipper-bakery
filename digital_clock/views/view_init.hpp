#pragma once

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
    void OnStageTimerTimeout();
    static void OnDraw(Canvas* canvas, const Model& model);

    void FinishSplash();

    outer_type* GetOuter() const;

private:
    cookie::ViewModel<Model> m_view;

    cookie::FuriEventLoopTimer m_splash_stage_timer;
    uint32_t m_splash_stage;
};
