#include "view_init.hpp"

#include "digital_clock_app.hpp"
#include "seven_segment_display.hpp"

#include <gui/canvas.h>

static constexpr uint32_t TEXT_SWITCH_PERIOD_MS = 1000;

InitView::InitView()
    : m_splash_stage_timer(
          get_outer()->GetEventLoop(),
          [](void* context) {
              InitView* view = reinterpret_cast<InitView*>(context);
              view->OnStageTimerTimeout();
          },
          FuriEventLoopTimerTypePeriodic,
          this) {
    view_set_context(*m_view, this);
    view_set_draw_callback(*m_view, [](Canvas* canvas, void* mdl) {
        const Model* model = reinterpret_cast<const Model*>(mdl);
        OnDraw(canvas, *model);
    });
    view_set_enter_callback(*m_view, [](void* context) {
        InitView* view = reinterpret_cast<InitView*>(context);
        view->OnEnter();
    });
    view_set_exit_callback(*m_view, [](void* context) {
        InitView* view = reinterpret_cast<InitView*>(context);
        view->OnExit();
    });
    view_set_input_callback(*m_view, [](InputEvent* event, void* context) {
        if(event->type == InputTypeShort && event->key == InputKeyOk) {
            InitView* view = reinterpret_cast<InitView*>(context);
            view->FinishSplash();
            return true;
        }
        return false;
    });
}

void InitView::OnEnter() {
    m_splash_stage = 0;
    furi_event_loop_timer_start(*m_splash_stage_timer, furi_ms_to_ticks(TEXT_SWITCH_PERIOD_MS));
}

void InitView::OnExit() {
    furi_event_loop_timer_stop(*m_splash_stage_timer);
}

void InitView::OnStageTimerTimeout() {
    m_splash_stage++;
    if(m_splash_stage == 1) {
        cookie::with_view_model(*m_view, [](Model& model) { model.display_second_line = true; });
    } else {
        FinishSplash();
    }
}

void InitView::OnDraw(Canvas* canvas, const Model& model) {
    canvas_clear(canvas);

    const char* text_to_display = model.display_second_line ? "ALARM" : "FLIPPER";
    const uint32_t text_width = SevenSegmentDisplay::DrawString(nullptr, text_to_display, 0, 0);

    SevenSegmentDisplay::DrawString(
        canvas,
        text_to_display,
        (128 - text_width) / 2,
        (64 - SevenSegmentDisplay::GetGlyphHeight()) / 2);
}

void InitView::FinishSplash() {
    furi_event_loop_timer_stop(*m_splash_stage_timer);
    get_outer()->SendAppEvent(AppLogicEvent::GoToNextScene);
}

IMPLEMENT_GET_OUTER(InitView);
