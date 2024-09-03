#include "view_clock.hpp"

#include <gui/canvas.h>

DigitalClockView::DigitalClockView() {
    view_set_context(*m_view, this);
    view_set_draw_callback(*m_view, OnDraw);
}

void DigitalClockView::OnDraw(Canvas* canvas, void* mdl) {
    Model* model = reinterpret_cast<Model*>(mdl);
    UNUSED(model);

    canvas_clear(canvas);
}
