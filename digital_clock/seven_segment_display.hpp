#pragma once

#include <furi/core/base.h>

struct Canvas;

class SevenSegmentDisplay {
public:
    // All public routines return the width of the element drawn, with padding
    // Passing a null canvas draws nothing and returns the width of the elements that would have been drawn
    static uint32_t DrawNumberBCD(Canvas* canvas, uint32_t num_bcd, uint32_t x, uint32_t y);
    static uint32_t DrawColon(Canvas* canvas, uint32_t x, uint32_t y);

private:
    static uint32_t DrawDigit(Canvas* canvas, uint8_t digit, uint32_t x, uint32_t y);
    static void DrawHorizontalSegment(Canvas* canvas, uint32_t x, uint32_t y);
    static void DrawVerticalSegment(Canvas* canvas, uint32_t x, uint32_t y);
};
