#pragma once

#include <cookie/within>
#include <cookie/gui/view>

class DigitalClockApp;

class DigitalClockView : cookie::Within<DigitalClockView, DigitalClockApp> {
public:
    DigitalClockView();

    ::View* View() const {
        return m_view.get();
    }

private:
    struct Model {
        uint8_t hour, minute, second;
    };

    void OnEnter();
    static void OnDraw(Canvas* canvas, const Model* model);

    // 7-segment-display routines and constants
    static void DrawSevenSegmentNumber(Canvas* canvas, uint32_t num, uint32_t x, uint32_t y);
    static void DrawSevenSegmentDigit(Canvas* canvas, uint8_t digit, uint32_t x, uint32_t y);
    static void DrawHorizontalSegment(Canvas* canvas, uint32_t x, uint32_t y);
    static void DrawVerticalSegment(Canvas* canvas, uint32_t x, uint32_t y);
    static void DrawColon(Canvas* canvas, uint32_t x, uint32_t y);

private:
    outer_type* GetOuter() const;

private:
    cookie::View m_view;
};
