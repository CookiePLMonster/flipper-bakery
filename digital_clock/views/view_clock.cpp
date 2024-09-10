#include "view_clock.hpp"

#include "digital_clock_app.hpp"
#include "seven_segment_display.hpp"

#include <gui/canvas.h>
#include <stm32wbxx_ll_rtc.h>

#include <furi_hal_rtc.h>

#include <cookie/common>

static constexpr uint32_t TIME_UPDATE_PERIOD_MS = 500;

DigitalClockView::DigitalClockView()
    : m_view(furi_hal_rtc_get_locale_timeformat() == FuriHalRtcLocaleTimeFormat12h)
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
    view_set_custom_callback(*m_view, [](uint32_t event, void* context) {
        DigitalClockView* view = reinterpret_cast<DigitalClockView*>(context);
        if(event == cookie::furi_enum_param(AppLogicEvent::TickTock)) {
            view->OnTimeUpdate();
        }
        return false;
    });
}

void DigitalClockView::OnEnter() {
    OnTimeUpdate();
}

void DigitalClockView::OnExit() {
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
        model.hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(time));
        model.minute = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(time));
        model.second = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(time));
        model.tenths_of_second = tenths_of_second;
    });
}

void DigitalClockView::OnDraw(Canvas* canvas, const Model& model) {
    canvas_clear(canvas);

    int32_t cur_x;
    const int32_t cur_y = 24;

    const bool show_colon = model.tenths_of_second < 5;

    bool is_pm = model.hour >= 12;
    if(!model.twelve_hour_clock) {
        cur_x = 4;
        cur_x += SevenSegmentDisplay::DrawNumber(canvas, model.hour, cur_x, cur_y, 2);
    } else {
        uint8_t hour_12h = model.hour % 12;
        if(hour_12h == 0) {
            hour_12h = 12;
        }
        cur_x = 0;
        cur_x += SevenSegmentDisplay::DrawNumber(canvas, hour_12h, cur_x, cur_y, 2, true);
    }
    cur_x += SevenSegmentDisplay::DrawColon(show_colon ? canvas : nullptr, cur_x, cur_y);

    cur_x += SevenSegmentDisplay::DrawNumber(canvas, model.minute, cur_x, cur_y, 2);
    cur_x += SevenSegmentDisplay::DrawColon(show_colon ? canvas : nullptr, cur_x, cur_y);

    cur_x += SevenSegmentDisplay::DrawNumber(canvas, model.second, cur_x, cur_y, 2);
    if(model.twelve_hour_clock) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(
            canvas, 128, cur_y - 1, AlignRight, AlignBottom, is_pm ? "PM" : "AM");
    }
}

IMPLEMENT_GET_OUTER(DigitalClockView);
