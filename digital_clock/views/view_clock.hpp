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

public:
    struct Model {
        uint8_t hour, minute, second;

        void OnDraw(Canvas* canvas) const;
    };

    void OnEnter();

private:
    outer_type* GetOuter() const;

private:
    cookie::View m_view;
};
