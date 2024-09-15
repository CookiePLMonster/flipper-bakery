#include "view_init.hpp"

#include "digital_clock_app.hpp"
#include "seven_segment_display.hpp"

#include <gui/canvas.h>

#include <chrono>
#include <cookie/common>

static constexpr uint32_t TEXT_SWITCH_PERIOD_MS = 1000;

InitView::InitView()
    : m_splash_timer(get_outer()->GetEventLoop()) {
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
            view->m_splash_timer.wake_up(true);
            return true;
        }
        return false;
    });
}

void InitView::OnEnter() {
    m_splash_task = ProcessSplashAsync();
}

void InitView::OnExit() {
    m_splash_task.reset();
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

cookie::Task<> InitView::ProcessSplashAsync() {
    using namespace std::chrono_literals;

    // Don't bother updating the model if we skipped the wait
    if(co_await m_splash_timer.await_delay(cookie::furi_chrono_duration_to_ticks(1s))) {
        cookie::with_view_model(*m_view, [](Model& model) { model.display_second_line = true; });

        co_await m_splash_timer.await_delay(cookie::furi_chrono_duration_to_ticks(1s));
    }
    get_outer()->SendAppEvent(AppLogicEvent::GoToNextScene);
}

IMPLEMENT_GET_OUTER(InitView);
