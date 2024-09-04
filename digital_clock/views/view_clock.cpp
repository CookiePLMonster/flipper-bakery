#include "view_clock.hpp"

#include "digital_clock_app.hpp"

#include <gui/canvas.h>

DigitalClockView::DigitalClockView() {
    cookie::view_emplace_model<Model>(*m_view);

    view_set_context(*m_view, this);
    view_set_draw_callback(*m_view, [](Canvas* canvas, void* mdl) {
        Model* model = reinterpret_cast<Model*>(mdl);
        model->OnDraw(canvas);
    });
    view_set_enter_callback(*m_view, [](void* context) {
        DigitalClockView* view = reinterpret_cast<DigitalClockView*>(context);
        view->OnEnter();
    });
}

void DigitalClockView::OnEnter() {
    cookie::with_view_model(*m_view, [](Model& model) {
        model.hour = 12;
        model.minute = 34;
        model.second = 56;
    });
}

void DigitalClockView::Model::OnDraw(Canvas* canvas) const {
    canvas_clear(canvas);
}

IMPLEMENT_GET_OUTER(DigitalClockView);
