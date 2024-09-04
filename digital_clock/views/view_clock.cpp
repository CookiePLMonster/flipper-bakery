#include "view_clock.hpp"

#include "digital_clock_app.hpp"

#include <gui/canvas.h>
#include <lib/datetime/datetime.h>
#include <furi_hal_rtc.h>

static constexpr uint32_t TIME_UPDATE_PERIOD_MS = 500;

DigitalClockView::DigitalClockView()
    : m_time_update_timer(
          [](void* context) {
              DigitalClockView* view = reinterpret_cast<DigitalClockView*>(context);
              view->OnTimeUpdate();
          },
          FuriTimerTypePeriodic,
          this) {
    cookie::view_emplace_model<Model>(*m_view);

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
    DateTime current_time;
    furi_hal_rtc_get_datetime(&current_time);

    cookie::with_view_model(*m_view, [&](Model& model) {
        bool time_changed = false;
        time_changed |= std::exchange(model.hour, current_time.hour) != current_time.hour;
        time_changed |= std::exchange(model.minute, current_time.minute) != current_time.minute;
        time_changed |= std::exchange(model.second, current_time.second) != current_time.second;
        model.show_colon = time_changed;
    });
}

// 7 segment display dimensions
static constexpr uint32_t HOR_BOX_LENGTH = 8;
static constexpr uint32_t VERT_BOX_LENGTH = 12;
static constexpr uint32_t HOR_SEGMENT_LENGTH = HOR_BOX_LENGTH + 2; // plus dots
static constexpr uint32_t VERT_SEGMENT_LENGTH = VERT_BOX_LENGTH + 2;
static constexpr uint32_t SEGMENT_SLANT = 1;
static constexpr uint32_t SEGMENT_THICKNESS = 3;
static constexpr uint32_t COLON_THICKNESS = 2;
static constexpr uint32_t SEGMENT_NOTCH = 2;
static constexpr uint32_t DIGIT_GAP = 4;
static constexpr uint32_t COLON_GAP = DIGIT_GAP + 1;

static constexpr uint32_t DIGIT_SPACING = HOR_SEGMENT_LENGTH + (DIGIT_GAP * 2);

void DigitalClockView::OnDraw(Canvas* canvas, const Model* model) {
    canvas_clear(canvas);

    uint32_t cur_x = 4;
    const uint32_t cur_y = 24;

    DrawSevenSegmentNumber(canvas, model->hour, cur_x, cur_y);
    cur_x += (DIGIT_SPACING * 2);
    if(model->show_colon) {
        DrawColon(canvas, cur_x, cur_y);
    }
    cur_x += COLON_GAP;

    DrawSevenSegmentNumber(canvas, model->minute, cur_x, cur_y);
    cur_x += (DIGIT_SPACING * 2);
    if(model->show_colon) {
        DrawColon(canvas, cur_x, cur_y);
    }
    cur_x += COLON_GAP;

    DrawSevenSegmentNumber(canvas, model->second, cur_x, cur_y);
}

void DigitalClockView::DrawSevenSegmentNumber(Canvas* canvas, uint32_t num, uint32_t x, uint32_t y) {
    const uint8_t first = num / 10;
    const uint8_t second = num % 10;
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
        y + (VERT_SEGMENT_LENGTH * 2) + SEGMENT_NOTCH - colon_height,
        COLON_THICKNESS,
        COLON_THICKNESS);
}

IMPLEMENT_GET_OUTER(DigitalClockView);
