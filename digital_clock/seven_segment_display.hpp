#pragma once

#include <furi/core/base.h>

struct Canvas;

class SevenSegmentDisplay {
public:
    // All public routines return the width of the element drawn, with padding
    // Passing a null canvas draws nothing and returns the width of the elements that would have been drawn
    static uint32_t DrawNumberBCD(Canvas* canvas, uint32_t num_bcd, int32_t x, int32_t y);
    static uint32_t DrawColon(Canvas* canvas, int32_t x, int32_t y);

    // TODO: cookie::StringView, for now use a C string
    static uint32_t DrawString(Canvas* canvas, const char* str, int32_t x, int32_t y);
    static uint32_t DrawChar(Canvas* canvas, char ch, int32_t x, int32_t y);

    static uint32_t GetGlyphHeight();

private:
    static uint32_t DrawDigit(Canvas* canvas, uint8_t digit, int32_t x, int32_t y);
    static uint32_t DrawGlyph(Canvas* canvas, uint8_t code, int32_t, int32_t y);
    static void DrawHorizontalSegment(Canvas* canvas, int32_t x, int32_t y);
    static void DrawVerticalSegment(Canvas* canvas, int32_t x, int32_t y);
};
