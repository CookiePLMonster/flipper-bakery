#include "view_init.hpp"

#include "digital_clock_app.hpp"
#include "seven_segment_display.hpp"

#include <gui/canvas.h>

static constexpr uint32_t TEXT_SWITCH_PERIOD_MS = 1000;

InitView::InitView() {
    view_set_context(*m_view, this);
    view_set_draw_callback(*m_view, [](Canvas* canvas, void* mdl) {
        const Model* model = reinterpret_cast<const Model*>(mdl);
        OnDraw(canvas, *model);
    });
    view_set_enter_callback(*m_view, [](void* context) {
        InitView* view = reinterpret_cast<InitView*>(context);
        view->OnEnter();
    });
    view_set_input_callback(*m_view, [](InputEvent* event, void* context) {
        if(event->type == InputTypeShort && event->key == InputKeyOk) {
            InitView* view = reinterpret_cast<InitView*>(context);
            view->m_splash_task.Skip();
            return true;
        }
        return false;
    });
}

void InitView::OnEnter() {
    m_splash_task = ProcessSplashAsync(get_outer()->GetEventLoop());
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

auto InitView::ProcessSplashAsync([[maybe_unused]] ::FuriEventLoop* event_loop) -> SplashTask {
    // Don't bother updating the model if we skipped the wait
    if(co_yield std::chrono::seconds(1)) {
        cookie::with_view_model(*m_view, [](Model& model) { model.display_second_line = true; });

        co_yield std::chrono::seconds(1);
    }
    get_outer()->SendAppEvent(AppLogicEvent::GoToNextScene);
}

IMPLEMENT_GET_OUTER(InitView);
