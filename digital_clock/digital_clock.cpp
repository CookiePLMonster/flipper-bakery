#include <furi.h>

extern "C" int32_t digital_clock_app(void* p) {
    UNUSED(p);
    FURI_LOG_I("TEST", "Hello world");
    FURI_LOG_I("TEST", "I'm digital_clock!");

    return 0;
}
