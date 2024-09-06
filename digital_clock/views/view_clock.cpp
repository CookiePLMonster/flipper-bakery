#include "view_clock.hpp"

#include "digital_clock_app.hpp"
#include "seven_segment_display.hpp"

#include <gui/canvas.h>
#include <stm32wbxx_ll_rtc.h>

#include <cookie/common>

static constexpr uint32_t TIME_UPDATE_PERIOD_MS = 500;

DigitalClockView::DigitalClockView()
    : m_time_update_timer(
          [](void* context) {
              DigitalClockView* view = reinterpret_cast<DigitalClockView*>(context);
              view->OnTimeUpdate();
          },
          FuriTimerTypePeriodic,
          this)
    , PREDIV_S(LL_RTC_GetSynchPrescaler(RTC)) {
    view_set_context(*m_view, this);
    view_set_draw_callback(*m_view, [](Canvas* canvas, void* mdl) {
        const Model* model = reinterpret_cast<const Model*>(mdl);
        OnDraw(canvas, *model);
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
    furi_timer_start(*m_time_update_timer, furi_ms_to_ticks(TIME_UPDATE_PERIOD_MS));
    OnTimeUpdate();
}

void DigitalClockView::OnExit() {
    furi_timer_stop(*m_time_update_timer);
}

void DigitalClockView::OnTimeUpdate() {
    // STM32L496xx errata stated that RTC time needs to be read twice as the lock may fail,
    // but the STM32WB55Rx errata does not state this. While the earlier version of this code
    // implemented the loop, now we're hoping that a single read is enough.
    uint32_t subsecond, time;
    {
        cookie::ScopedFuriCritical critical;
        subsecond = LL_RTC_TIME_GetSubSecond(RTC);
        time = LL_RTC_TIME_Get(RTC);
        LL_RTC_DATE_Get(RTC); // Unlock the shadow registers
    }

    const uint8_t tenths_of_second = ((PREDIV_S - subsecond) * 10) / (PREDIV_S + 1);

    cookie::with_view_model(*m_view, [time, tenths_of_second](Model& model) {
        model.hour_bcd = __LL_RTC_GET_HOUR(time);
        model.minute_bcd = __LL_RTC_GET_MINUTE(time);
        model.second_bcd = __LL_RTC_GET_SECOND(time);
        model.tenths_of_second = tenths_of_second;
    });
}

void DigitalClockView::OnDraw(Canvas* canvas, const Model& model) {
    canvas_clear(canvas);

    int32_t cur_x = 4;
    const int32_t cur_y = 24;

    const bool show_colon = model.tenths_of_second < 5;

    cur_x += SevenSegmentDisplay::DrawNumberBCD(canvas, model.hour_bcd, cur_x, cur_y);
    cur_x += SevenSegmentDisplay::DrawColon(show_colon ? canvas : nullptr, cur_x, cur_y);

    cur_x += SevenSegmentDisplay::DrawNumberBCD(canvas, model.minute_bcd, cur_x, cur_y);
    cur_x += SevenSegmentDisplay::DrawColon(show_colon ? canvas : nullptr, cur_x, cur_y);

    cur_x += SevenSegmentDisplay::DrawNumberBCD(canvas, model.second_bcd, cur_x, cur_y);
}

IMPLEMENT_GET_OUTER(DigitalClockView);
