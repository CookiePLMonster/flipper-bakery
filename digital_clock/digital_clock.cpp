#include <furi.h>

#include "digital_clock_app.hpp"

extern "C" int32_t digital_clock_app(void* p) {
    UNUSED(p);

    DigitalClockApp app;
    app.Run();

    return 0;
}
