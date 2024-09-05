#pragma once

#include <cookie/timer>
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

private:
    cookie::ViewModel<Model> m_view{cookie::construct_model_tag};

    // TODO: Use FuriEventLoopTimer when it's available
    cookie::FuriTimer m_text_switch_timer;
};
