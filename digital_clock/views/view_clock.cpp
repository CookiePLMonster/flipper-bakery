#include "view_clock.hpp"

#include "digital_clock_app.hpp"
#include "seven_segment_display.hpp"

#include <gui/canvas.h>
#include <stm32wbxx_ll_rtc.h>

static constexpr uint32_t TIME_UPDATE_PERIOD_MS = 500;

DigitalClockView::DigitalClockView()
    : m_time_update_timer(
          [](void* context) {
              DigitalClockView* view = reinterpret_cast<DigitalClockView*>(context);
              view->OnTimeUpdate();
          },
          FuriTimerTypePeriodic,
          this) {
    view_set_context(*m_view, this);
    view_set_draw_callback(*m_view, [](Canvas* canvas, void* mdl) {
        const Model* model = reinterpret_cast<const Model*>(mdl);
        OnDraw(canvas, model);
    });
    view_set_enter_callback(*m_view, [](void* context) {
        DigitalClockView* view = reinterpret_cast<DigitalClockView*>(context);
        view->OnEnter();
    });
    view_set_exit_callback(*m_view, [](void* context) {
        DigitalClockView* view = reinterpret_cast<DigitalClockView*>(context);
        view->OnExit();
    });
}

void DigitalClockView::OnEnter() {
    OnTimeUpdate();

    furi_timer_start(*m_time_update_timer, furi_ms_to_ticks(TIME_UPDATE_PERIOD_MS));
}

void DigitalClockView::OnExit() {
    furi_timer_stop(*m_time_update_timer);
}

void DigitalClockView::OnTimeUpdate() {
    FURI_CRITICAL_ENTER();

    const uint32_t PREDIV_S = LL_RTC_GetSynchPrescaler(RTC);
    const uint32_t time = LL_RTC_TIME_Get(RTC);

    // Wait for the sub-second value to become safe to query
    while(LL_RTC_IsActiveFlag_SHP(RTC)) {
    }
    const uint32_t subsecond = LL_RTC_TIME_GetSubSecond(RTC);

    FURI_CRITICAL_EXIT();

    const uint8_t tenths_of_second = ((PREDIV_S - subsecond) * 10) / (PREDIV_S + 1);

    cookie::with_view_model(*m_view, [time, tenths_of_second](Model& model) {
        model.hour_bcd = __LL_RTC_GET_HOUR(time);
        model.minute_bcd = __LL_RTC_GET_MINUTE(time);
        model.second_bcd = __LL_RTC_GET_SECOND(time);
        model.tenths_of_second = tenths_of_second;
    });
}

void DigitalClockView::OnDraw(Canvas* canvas, const Model* model) {
    canvas_clear(canvas);

    uint32_t cur_x = 4;
    const uint32_t cur_y = 24;

    const bool show_colon = model->tenths_of_second < 5;

    cur_x += SevenSegmentDisplay::DrawNumberBCD(canvas, model->hour_bcd, cur_x, cur_y);
    cur_x += SevenSegmentDisplay::DrawColon(show_colon ? canvas : nullptr, cur_x, cur_y);

    cur_x += SevenSegmentDisplay::DrawNumberBCD(canvas, model->minute_bcd, cur_x, cur_y);
    cur_x += SevenSegmentDisplay::DrawColon(show_colon ? canvas : nullptr, cur_x, cur_y);

    cur_x += SevenSegmentDisplay::DrawNumberBCD(canvas, model->second_bcd, cur_x, cur_y);
}

IMPLEMENT_GET_OUTER(DigitalClockView);
