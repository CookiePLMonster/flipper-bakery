#include "seven_segment_display.hpp"

#include <gui/canvas.h>

static constexpr uint32_t HOR_BOX_LENGTH = 8;
static constexpr uint32_t VERT_BOX_LENGTH = 12;
static constexpr uint32_t HOR_SEGMENT_LENGTH = HOR_BOX_LENGTH + 2; // plus dots
static constexpr uint32_t VERT_SEGMENT_LENGTH = VERT_BOX_LENGTH + 2;
static constexpr uint32_t SEGMENT_SLANT = 1;
static constexpr uint32_t SEGMENT_THICKNESS = 3;
static constexpr uint32_t COLON_THICKNESS = 3;
static constexpr uint32_t SEGMENT_NOTCH = 2;
static constexpr uint32_t DIGIT_GAP = 4;

static constexpr uint32_t GLYPH_SPACING = HOR_SEGMENT_LENGTH + (DIGIT_GAP * 2);
static constexpr uint32_t COLON_SPACING = COLON_THICKNESS * 2;

uint32_t
    SevenSegmentDisplay::DrawNumberBCD(Canvas* canvas, uint8_t num_bcd, int32_t x, int32_t y) {
    const uint8_t first = (num_bcd >> 4) & 0xF;
    const uint8_t second = num_bcd & 0xF;
    uint32_t result = DrawDigit(canvas, first, x, y);
    result += DrawDigit(canvas, second, x + GLYPH_SPACING, y);
    return result;
}

uint32_t SevenSegmentDisplay::DrawNumber(
    Canvas* canvas,
    uint32_t num,
    int32_t x,
    int32_t y,
    uint8_t min_digits,
    bool pad_with_space) {
    uint32_t result = 0;

    // First count the digits and pad with either zeroes or spaces
    uint8_t total_digits = 0;
    {
        uint32_t temp_num = num;
        do {
            total_digits++;
            temp_num /= 10;
        } while(temp_num != 0);
    }

    if(total_digits < min_digits) {
        const uint8_t num_padding = min_digits - total_digits;
        if(pad_with_space) {
            const uint32_t spacing = num_padding * GLYPH_SPACING;
            result += spacing;
            x += spacing;
        } else {
            for(uint8_t i = 0; i < num_padding; ++i) {
                result += DrawDigit(canvas, 0, x, y);
                x += GLYPH_SPACING;
            }
        }
    }

    // Draw from the right
    x += (GLYPH_SPACING * total_digits);

    do {
        const uint8_t digit = num % 10;
        num /= 10;
        result += DrawDigit(canvas, digit, x - GLYPH_SPACING, y);
        x -= GLYPH_SPACING;
    } while(num != 0);
    return result;
}

uint32_t SevenSegmentDisplay::DrawDigit(Canvas* canvas, uint8_t digit, int32_t x, int32_t y) {
    // TODO: cookie_debug_check(digit < 10);
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
    return DrawGlyph(canvas, DISPLAY_CODING_CHARACTERS[digit], x, y);
}

uint32_t SevenSegmentDisplay::DrawGlyph(Canvas* canvas, uint8_t code, int32_t x, int32_t y) {
    if(canvas) {
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
    return GLYPH_SPACING;
}

void SevenSegmentDisplay::DrawHorizontalSegment(Canvas* canvas, int32_t x, int32_t y) {
    canvas_draw_box(canvas, x + 1, y, HOR_SEGMENT_LENGTH - 2, SEGMENT_THICKNESS);
    canvas_draw_dot(canvas, x, y + 1);
    canvas_draw_dot(canvas, x + HOR_SEGMENT_LENGTH - 1, y + 1);
}

void SevenSegmentDisplay::DrawVerticalSegment(Canvas* canvas, int32_t x, int32_t y) {
    const uint32_t PART_LENGTH = (VERT_SEGMENT_LENGTH - 2) / 2;
    canvas_draw_box(canvas, x + SEGMENT_SLANT, y + 1, SEGMENT_THICKNESS, PART_LENGTH);
    canvas_draw_box(canvas, x, y + PART_LENGTH + 1, SEGMENT_THICKNESS, PART_LENGTH);
    canvas_draw_dot(canvas, x + SEGMENT_SLANT + 1, y);
    canvas_draw_dot(canvas, x + 1, y + VERT_SEGMENT_LENGTH - 1);
}

uint32_t SevenSegmentDisplay::DrawColon(Canvas* canvas, int32_t x, int32_t y) {
    if(canvas) {
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
    return COLON_SPACING;
}

uint32_t SevenSegmentDisplay::DrawString(Canvas* canvas, const char* str, int32_t x, int32_t y) {
    uint32_t width = 0;
    while(*str != '\0') {
        width += DrawChar(canvas, *str++, x + width, y);
    }
    return width;
}

uint32_t SevenSegmentDisplay::DrawChar(Canvas* canvas, char ch, int32_t x, int32_t y) {
    if(ch >= '0' && ch <= '9') {
        return DrawDigit(canvas, ch - '0', x, y);
    }
    if(ch == ':') {
        return DrawColon(canvas, x, y);
    }

    // M and W occupy two cells, special case them
    if(ch == 'M' || ch == 'm') {
        const uint32_t left = DrawGlyph(canvas, 0b1100110, x, y);
        const uint32_t right = DrawGlyph(canvas, 0b1110010, x + left, y);
        return left + right;
    }
    if(ch == 'W' || ch == 'w') {
        const uint32_t left = DrawGlyph(canvas, 0b0011110, x, y);
        const uint32_t right = DrawGlyph(canvas, 0b0111100, x + left, y);
        return left + right;
    }

    if(ch >= 'A' && ch <= 'Z') {
        static const uint8_t DISPLAY_CODING_CHARACTERS[26] = {
            0b1110111, // A
            0b0011111, // b (B missing)
            0b1001110, // C
            0b0111101, // d (D missing)
            0b1001111, // E
            0b1000111, // F
            0b1011110, // G
            0b0110111, // H
            0b0000110, // I
            0b0111100, // J
            0b0000000, // K and k missing
            0b0001110, // L
            0b0000000, // M special cased
            0b1110110, // N
            0b1111110, // O
            0b1100111, // P
            0b1110011, // q (Q missing)
            0b0000101, // r (R missing)
            0b1011011, // S
            0b0001111, // t (T missing)
            0b0111110, // U
            0b0000000, // V and v missing
            0b0000000, // W special cased
            0b0000000, // X and x missing
            0b0111011, // y (Y missing)
            0b1101101, // Z
        };
        return DrawGlyph(canvas, DISPLAY_CODING_CHARACTERS[ch - 'A'], x, y);
    }

    if(ch >= 'a' && ch <= 'z') {
        static const uint8_t DISPLAY_CODING_CHARACTERS[26] = {
            0b1111101, // a
            0b0011111, // b
            0b0001101, // c
            0b0111101, // d
            0b1101111, // e
            0b1000111, // F (f missing)
            0b1111011, // g
            0b0010111, // h
            0b0010000, // i
            0b0111000, // j
            0b0000000, // K and k missing
            0b0000110, // l
            0b0000000, // M special cased
            0b0010101, // n
            0b0011101, // o
            0b1100111, // P (p missing)
            0b1110011, // q
            0b0000101, // r
            0b1011011, // S (s missing)
            0b0001111, // t
            0b0011100, // u
            0b0000000, // V and v missing
            0b0000000, // W special cased
            0b0000000, // X and x missing
            0b0111011, // y (Y missing)
            0b1101101, // Z (z missing)
        };
        return DrawGlyph(canvas, DISPLAY_CODING_CHARACTERS[ch - 'a'], x, y);
    }

    // Space and other unsupported characters will leave a narrow space
    return COLON_SPACING;
}

uint32_t SevenSegmentDisplay::GetGlyphHeight() {
    return (2 * VERT_SEGMENT_LENGTH) + 5;
}
