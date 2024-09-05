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

    // 7-segment-display routines and constants
    static void DrawSevenSegmentNumber(Canvas* canvas, uint32_t num_bcd, uint32_t x, uint32_t y);
    static void DrawSevenSegmentDigit(Canvas* canvas, uint8_t digit, uint32_t x, uint32_t y);
    static void DrawHorizontalSegment(Canvas* canvas, uint32_t x, uint32_t y);
    static void DrawVerticalSegment(Canvas* canvas, uint32_t x, uint32_t y);
    static void DrawColon(Canvas* canvas, uint32_t x, uint32_t y);

private:
    outer_type* GetOuter() const;

private:
    cookie::ViewModel<Model> m_view{cookie::construct_model_tag};

    // TODO: Use FuriEventLoopTimer when it's available
    cookie::FuriTimer m_time_update_timer;
};
