#include <furi.h>
#include <furi_hal_gpio.h>

#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <storage/storage.h>

#include "util/pbm.h"

#include <algorithm>

#define TAG "Mode7"

// This is simplified compared to the "traditional" bit band alias access, since we only occupy the bottom 256KB of SRAM anyway
#define BIT_BAND_ALIAS(var) \
    (reinterpret_cast<uint32_t*>((reinterpret_cast<uintptr_t>(var) << 5) | 0x22000000))

static constexpr uint32_t MAIN_VIEW = 0;

static constexpr int16_t SCREEN_WIDTH = 128;
static constexpr int16_t SCREEN_HEIGHT = 64;
static constexpr int16_t EYE_DISTANCE = 32;
static constexpr int16_t HORIZON = 20;
static constexpr uint8_t TILE_SIZE = 8;

static uint32_t exit_app(void*) {
    FURI_LOG_I("Test", "exit_app");
    return VIEW_NONE;
}

static int16_t g_offset_x = 0;
static int16_t g_offset_y = 0;

static Pbm* test_file;

static uint8_t* screen_buffer_space;
static uint8_t* back_buffer[2];
static uint8_t current_backbuffer = 0;

static FuriEventFlag* presentation_flag;

static void draw_test_checkerboard(Canvas* canvas, void* model) {
    UNUSED(model);

    const uint8_t buffer_to_use = current_backbuffer;
    furi_event_flag_clear(presentation_flag, 1 << buffer_to_use);
    canvas_draw_xbm(canvas, 0, 0, 128, 64, back_buffer[buffer_to_use]);
    furi_event_flag_set(presentation_flag, 1 << buffer_to_use);
}

/*static bool input_callback(InputEvent* event, void* context) {
    UNUSED(context);
    if(event->type == InputTypePress || event->type == InputTypeRepeat) {
        if(event->key == InputKeyUp) {
            g_offset_y--;
            return true;
        }
        if(event->key == InputKeyDown) {
            g_offset_y++;
            return true;
        }

        if(event->key == InputKeyLeft) {
            g_offset_x--;
            return true;
        }
        if(event->key == InputKeyRight) {
            g_offset_x++;
            return true;
        }
    }
    return false;
}*/

static void tick_callback(void* context) {
    View* view = static_cast<View*>(context);

    // TODO: This should react to events and cache the input buttons state, not this
    if(!furi_hal_gpio_read(&gpio_button_right)) {
        g_offset_x++;
    } else if(!furi_hal_gpio_read(&gpio_button_left)) {
        g_offset_x--;
    }
    if(!furi_hal_gpio_read(&gpio_button_down)) {
        g_offset_y++;
    } else if(!furi_hal_gpio_read(&gpio_button_up)) {
        g_offset_y--;
    }

    const uint8_t next_backbuffer = (current_backbuffer + 1) % 2;

    // "Rasterize" the background
    const int32_t background_width = test_file->width;
    const uint32_t background_pitch = pbm_get_pitch(test_file);
    const int32_t background_height = test_file->height;
    const uint32_t* background_bitmap = BIT_BAND_ALIAS(test_file->bitmap);

    furi_event_flag_wait(
        presentation_flag,
        1 << next_backbuffer,
        FuriFlagWaitAny | FuriFlagNoClear,
        FuriWaitForever);
    memset(back_buffer[next_backbuffer], 0, 16 * 64);

    // This is "slow" but simulates how backgrounds are rasterized. Use it for now.
    // This method also allows for easy repeat modes
    uint32_t* screen_bitmap = BIT_BAND_ALIAS(back_buffer[next_backbuffer]);
    for(int32_t y = -SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT / 2; ++y) {
        for(int32_t x = -SCREEN_WIDTH / 2; x < SCREEN_WIDTH / 2; ++x) {
            int32_t dx = x + (SCREEN_WIDTH / 2);
            int32_t dy = y + (SCREEN_HEIGHT / 2);

            int32_t px = x;
            int32_t py = EYE_DISTANCE;
            int32_t pz = y + HORIZON;

            float sx = static_cast<float>(px) / pz;
            float sy = static_cast<float>(py) / -pz;

            int32_t sampled_x = (sx * background_width) + g_offset_x;
            int32_t sampled_y = (sy * background_height) + g_offset_y;
            if(sampled_x >= 0 && sampled_x < background_width && sampled_y >= 0 &&
               sampled_y < background_height) {
                screen_bitmap[dy * 128 + dx] =
                    background_bitmap[(sampled_y * background_pitch) + sampled_x];
            }
        }
    }

    current_backbuffer = next_backbuffer;
    if(view != nullptr) {
        view_commit_model(view, true);
    }
}

extern "C" int32_t mode7_demo_app(void* p) {
    UNUSED(p);

    Storage* storage = static_cast<Storage*>(furi_record_open(RECORD_STORAGE));
    test_file = pbm_load_file(storage, APP_ASSETS_PATH("cookie_monster.pbm"));
    furi_record_close(RECORD_STORAGE);

    g_offset_x = 64; //64 - (test_file->width / 2);
    g_offset_y = 150;

    furi_timer_set_thread_priority(FuriTimerThreadPriorityElevated);

    presentation_flag = furi_event_flag_alloc();
    furi_event_flag_set(presentation_flag, 0xFFFFFFFFu);
    screen_buffer_space = static_cast<uint8_t*>(malloc(16 * 64 * 2));
    back_buffer[0] = screen_buffer_space;
    back_buffer[1] = screen_buffer_space + (16 * 64);

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
    //view_set_input_callback(test_view, input_callback);

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

    furi_delay_ms(1000);
    furi_record_close(RECORD_GUI);
    free(screen_buffer_space);
    furi_event_flag_free(presentation_flag);

    furi_timer_set_thread_priority(FuriTimerThreadPriorityNormal);

    pbm_free(test_file);

    return 0;
}
