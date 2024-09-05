#pragma once

#include <cookie/timer>
#include <cookie/within>
#include <cookie/gui/view>
#include <cookie/gui/view_dispatcher>

class DigitalClockApp;

class DigitalClockView : cookie::Within<DigitalClockView, DigitalClockApp> {
public:
    DigitalClockView();

    ::View* View() const {
        return m_view.get();
    }

private:
    struct Model {
        uint8_t hour_bcd, minute_bcd, second_bcd;
        uint8_t tenths_of_second;
    };

    void OnEnter();
    void OnExit();
    void OnTimeUpdate();
    static void OnDraw(Canvas* canvas, const Model* model);

private:
    outer_type* GetOuter() const;

private:
    cookie::ViewModel<Model> m_view{cookie::construct_model_tag};

    // TODO: Use FuriEventLoopTimer when it's available
    cookie::FuriTimer m_time_update_timer;
};
