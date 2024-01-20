#include <furi.h>

#include <gui/gui.h>
#include <gui/view_dispatcher.h>

#include <storage/storage.h>

#include "util/pbm.h"

static constexpr uint32_t MAIN_VIEW = 0;

static constexpr uint8_t SCREEN_WIDTH = 128;
static constexpr uint8_t SCREEN_HEIGHT = 64;
static constexpr uint8_t TILE_SIZE = 8;

static uint32_t exit_app(void*) {
    FURI_LOG_I("Test", "exit_app");
    return VIEW_NONE;
}

static Pbm* test_file;

static void draw_test_checkerboard(Canvas* canvas, void* model) {
    UNUSED(model);

    canvas_draw_xbm(canvas, 0, 0, test_file->width, test_file->height, test_file->bitmap);
}

/*static void tick_callback(void* context) {
    View* view = static_cast<View*>(context);

    if(view != nullptr) {
        view_commit_model(view, true);
    }
}*/

extern "C" int32_t mode7_demo_app(void* p) {
    UNUSED(p);

    Storage* storage = static_cast<Storage*>(furi_record_open(RECORD_STORAGE));
    test_file = pbm_load_file(storage, APP_ASSETS_PATH("cookie_monster.pbm"));
    furi_record_close(RECORD_STORAGE);

    Gui* gui = static_cast<Gui*>(furi_record_open(RECORD_GUI));
    UNUSED(gui);

    ViewDispatcher* view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(view_dispatcher);
    view_dispatcher_attach_to_gui(view_dispatcher, gui, ViewDispatcherTypeFullscreen);

    // This causes the issue unless the timer thread priority is bumped up
    //view_dispatcher_set_tick_event_callback(
    //    view_dispatcher, tick_callback, furi_kernel_get_tick_frequency() / 60);
    // furi_timer_set_thread_priority(FuriTimerThreadPriorityElevated);

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

    pbm_free(test_file);

    return 0;
}
