#pragma once

#include <cookie/timer>
#include <cookie/within>
#include <cookie/gui/view>
#include <cookie/gui/view_dispatcher>

class DigitalClockApp;

class DigitalClockView : cookie::Within<DigitalClockView, DigitalClockApp> {
public:
    DigitalClockView();

    const auto& View() const {
        return m_view;
    }

private:
    struct Model {
        Model(bool twelve_hour_clock)
            : twelve_hour_clock(twelve_hour_clock) {
        }

        uint8_t hour, minute, second;
        uint8_t tenths_of_second;

        const bool twelve_hour_clock;
    };

    void OnEnter();
    void OnTimeUpdate();
    static void OnDraw(Canvas* canvas, const Model& model);

private:
    outer_type* get_outer() const;

private:
    cookie::ViewModel<Model> m_view;

    const uint32_t PREDIV_S;
};
