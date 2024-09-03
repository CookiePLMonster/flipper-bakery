#pragma once

#include <cookie/gui/view>

class DigitalClockView {
public:
    DigitalClockView();

    ::View* View() const {
        return m_view.get();
    }

private:
    static void OnDraw(Canvas* canvas, void* mdl);

private:
    struct Model {};

    cookie::View m_view;
};
