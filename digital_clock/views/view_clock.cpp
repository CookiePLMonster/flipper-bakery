#include "view_clock.hpp"

#include "digital_clock_app.hpp"

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

// 7 segment display dimensions
static constexpr uint32_t HOR_BOX_LENGTH = 8;
static constexpr uint32_t VERT_BOX_LENGTH = 12;
static constexpr uint32_t HOR_SEGMENT_LENGTH = HOR_BOX_LENGTH + 2; // plus dots
static constexpr uint32_t VERT_SEGMENT_LENGTH = VERT_BOX_LENGTH + 2;
static constexpr uint32_t SEGMENT_SLANT = 1;
static constexpr uint32_t SEGMENT_THICKNESS = 3;
static constexpr uint32_t COLON_THICKNESS = 3;
static constexpr uint32_t SEGMENT_NOTCH = 2;
static constexpr uint32_t DIGIT_GAP = 4;
static constexpr uint32_t COLON_GAP = COLON_THICKNESS * 2;

static constexpr uint32_t DIGIT_SPACING = HOR_SEGMENT_LENGTH + (DIGIT_GAP * 2);

void DigitalClockView::OnDraw(Canvas* canvas, const Model* model) {
    canvas_clear(canvas);

    uint32_t cur_x = 4;
    const uint32_t cur_y = 24;

    const bool show_colon = model->tenths_of_second < 5;

    DrawSevenSegmentNumber(canvas, model->hour_bcd, cur_x, cur_y);
    cur_x += (DIGIT_SPACING * 2);
    if(show_colon) {
        DrawColon(canvas, cur_x, cur_y);
    }
    cur_x += COLON_GAP;

    DrawSevenSegmentNumber(canvas, model->minute_bcd, cur_x, cur_y);
    cur_x += (DIGIT_SPACING * 2);
    if(show_colon) {
        DrawColon(canvas, cur_x, cur_y);
    }
    cur_x += COLON_GAP;

    DrawSevenSegmentNumber(canvas, model->second_bcd, cur_x, cur_y);
}

void DigitalClockView::DrawSevenSegmentNumber(
    Canvas* canvas,
    uint32_t num_bcd,
    uint32_t x,
    uint32_t y) {
    const uint8_t first = (num_bcd >> 4) & 0xF;
    const uint8_t second = num_bcd & 0xF;
    DrawSevenSegmentDigit(canvas, first, x, y);
    DrawSevenSegmentDigit(canvas, second, x + DIGIT_SPACING, y);
}

void DigitalClockView::DrawSevenSegmentDigit(Canvas* canvas, uint8_t digit, uint32_t x, uint32_t y) {
    static const uint8_t DISPLAY_CODING_CHARACTERS[10] = {
        0b1111110, // 0
        0b0110000, // 1
        0b1101101, // 2
        0b1111001, // 3
        0b0110011, // 4
        0b1011011, // 5
        0b1011111, // 6
        0b1110000, // 7
        0b1111111, // 8
        0b1111011, // 9
    };

    // TODO: cookie_debug_check(digit < 10);
    const uint8_t code = DISPLAY_CODING_CHARACTERS[digit];

    // A
    if((code & 0b1000000) != 0) {
        DrawHorizontalSegment(canvas, x + (SEGMENT_SLANT * 2) + SEGMENT_THICKNESS, y);
    }

    // B
    if((code & 0b100000) != 0) {
        DrawVerticalSegment(
            canvas, x + HOR_SEGMENT_LENGTH + (SEGMENT_SLANT * 2) + 1, y + SEGMENT_NOTCH);
    }

    // C
    if((code & 0b10000) != 0) {
        DrawVerticalSegment(
            canvas,
            x + HOR_SEGMENT_LENGTH + SEGMENT_SLANT + 1,
            y + VERT_SEGMENT_LENGTH + SEGMENT_THICKNESS);
    }

    // D
    if((code & 0b1000) != 0) {
        DrawHorizontalSegment(
            canvas, x + SEGMENT_THICKNESS, y + (VERT_SEGMENT_LENGTH * 2) + SEGMENT_NOTCH);
    }

    // E
    if((code & 0b100) != 0) {
        DrawVerticalSegment(
            canvas, x + SEGMENT_SLANT, y + VERT_SEGMENT_LENGTH + SEGMENT_THICKNESS);
    }

    // F
    if((code & 0b10) != 0) {
        DrawVerticalSegment(canvas, x + (SEGMENT_SLANT * 2), y + SEGMENT_NOTCH);
    }

    // G
    if((code & 0b1) != 0) {
        DrawHorizontalSegment(
            canvas, x + SEGMENT_SLANT + SEGMENT_THICKNESS, y + VERT_SEGMENT_LENGTH + 1);
    }
}

void DigitalClockView::DrawHorizontalSegment(Canvas* canvas, uint32_t x, uint32_t y) {
    canvas_draw_box(canvas, x + 1, y, HOR_SEGMENT_LENGTH - 2, SEGMENT_THICKNESS);
    canvas_draw_dot(canvas, x, y + 1);
    canvas_draw_dot(canvas, x + HOR_SEGMENT_LENGTH - 1, y + 1);
}

void DigitalClockView::DrawVerticalSegment(Canvas* canvas, uint32_t x, uint32_t y) {
    const uint32_t PART_LENGTH = (VERT_SEGMENT_LENGTH - 2) / 2;
    canvas_draw_box(canvas, x + SEGMENT_SLANT, y + 1, SEGMENT_THICKNESS, PART_LENGTH);
    canvas_draw_box(canvas, x, y + PART_LENGTH + 1, SEGMENT_THICKNESS, PART_LENGTH);
    canvas_draw_dot(canvas, x + SEGMENT_SLANT + 1, y);
    canvas_draw_dot(canvas, x + 1, y + VERT_SEGMENT_LENGTH - 1);
}

void DigitalClockView::DrawColon(Canvas* canvas, uint32_t x, uint32_t y) {
    const uint32_t colon_height = (VERT_SEGMENT_LENGTH / 2);
    canvas_draw_box(
        canvas,
        x + SEGMENT_SLANT + 1,
        y + colon_height + (COLON_THICKNESS / 2),
        COLON_THICKNESS,
        COLON_THICKNESS);
    canvas_draw_box(
        canvas,
        x + 1,
        y + (VERT_SEGMENT_LENGTH * 2) - colon_height + (COLON_THICKNESS / 2),
        COLON_THICKNESS,
        COLON_THICKNESS);
}

IMPLEMENT_GET_OUTER(DigitalClockView);
