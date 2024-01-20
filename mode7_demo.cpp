#include <furi.h>

#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <storage/storage.h>

#include "util/pbm.h"

#include <algorithm>

// This is simplified compared to the "traditional" bit band alias access, since we only occupy the bottom 256KB of SRAM anyway
#define BIT_BAND_ALIAS(var) \
    (reinterpret_cast<uint32_t*>((reinterpret_cast<uintptr_t>(var) << 5) | 0x22000000))

static constexpr uint32_t MAIN_VIEW = 0;

static constexpr uint8_t SCREEN_WIDTH = 128;
static constexpr uint8_t SCREEN_HEIGHT = 64;
static constexpr uint8_t TILE_SIZE = 8;

static uint32_t exit_app(void*) {
    FURI_LOG_I("Test", "exit_app");
    return VIEW_NONE;
}

static Pbm* test_file;

static uint8_t* screen_buffer;

static void draw_test_checkerboard(Canvas* canvas, void* model) {
    UNUSED(model);

    canvas_draw_xbm(canvas, 0, 0, 128, 64, screen_buffer);
}

static void tick_callback(void* context) {
    View* view = static_cast<View*>(context);

    // "Rasterize" the background
    const uint32_t background_width = test_file->width;
    const uint32_t background_pitch = pbm_get_pitch(test_file);
    const uint32_t background_height = std::min<uint16_t>(test_file->height, 64);
    const uint32_t* background_bitmap = BIT_BAND_ALIAS(test_file->bitmap);

    const uint16_t background_offset = 64 - (test_file->width / 2);
    uint32_t* screen_bitmap = BIT_BAND_ALIAS(screen_buffer) + background_offset;

    for(uint32_t y = 0; y < background_height; ++y) {
        for(uint32_t x = 0; x < background_width; ++x) {
            screen_bitmap[x] = background_bitmap[x];
        }
        background_bitmap += background_pitch;
        screen_bitmap += 128;
    }

    if(view != nullptr) {
        view_commit_model(view, true);
    }
}

extern "C" int32_t mode7_demo_app(void* p) {
    UNUSED(p);

    Storage* storage = static_cast<Storage*>(furi_record_open(RECORD_STORAGE));
    test_file = pbm_load_file(storage, APP_ASSETS_PATH("cookie_monster.pbm"));
    furi_record_close(RECORD_STORAGE);

    furi_timer_set_thread_priority(FuriTimerThreadPriorityElevated);

    screen_buffer = static_cast<uint8_t*>(malloc(16 * 64));

    Gui* gui = static_cast<Gui*>(furi_record_open(RECORD_GUI));
    UNUSED(gui);

    ViewDispatcher* view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(view_dispatcher);
    view_dispatcher_attach_to_gui(view_dispatcher, gui, ViewDispatcherTypeFullscreen);

    // This causes the issue unless the timer thread priority is bumped up
    view_dispatcher_set_tick_event_callback(
        view_dispatcher, tick_callback, furi_kernel_get_tick_frequency() / 60);

    View* test_view = view_alloc();
    view_set_previous_callback(test_view, exit_app);
    view_set_draw_callback(test_view, draw_test_checkerboard);

    view_dispatcher_set_event_callback_context(view_dispatcher, test_view);

    view_dispatcher_add_view(view_dispatcher, MAIN_VIEW, test_view);
    view_dispatcher_switch_to_view(view_dispatcher, MAIN_VIEW);

    // The timer interval of 1 causes issues - possibly starves all the other timers, incl. the input timer
    //FuriTimer* tick_timer = furi_timer_alloc(tick_callback, FuriTimerTypePeriodic, test_view);
    //furi_timer_start(tick_timer, 1);

    view_dispatcher_run(view_dispatcher);

    //furi_timer_stop(tick_timer);
    //furi_timer_free(tick_timer);
    view_dispatcher_remove_view(view_dispatcher, MAIN_VIEW);
    view_free(test_view);
    view_dispatcher_free(view_dispatcher);

    furi_record_close(RECORD_GUI);
    free(screen_buffer);

    furi_timer_set_thread_priority(FuriTimerThreadPriorityNormal);

    pbm_free(test_file);

    return 0;
}
