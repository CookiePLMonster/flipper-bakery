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
        uint8_t hour_bcd, minute_bcd, second_bcd;
        uint8_t tenths_of_second;
    };

    void OnEnter();
    void OnExit();
    void OnTimeUpdate();
    static void OnDraw(Canvas* canvas, const Model& model);

private:
    outer_type* GetOuter() const;

private:
    cookie::ViewModel<Model> m_view;

    const uint32_t PREDIV_S;
};
