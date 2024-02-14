#include <furi.h>
#include <furi_hal_gpio.h>

#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <storage/storage.h>

#include <toolbox/stream/file_stream.h>

#include "util/pbm.h"

#include <cinttypes>

#include <array>
#include <utility>

#include "engine/game_engine.h"

#define TAG "Mode7"

// This is simplified compared to the "traditional" bit band alias access, since we only occupy the bottom 256KB of SRAM anyway
#define BIT_BAND_ALIAS(var) \
    (reinterpret_cast<uint32_t*>((reinterpret_cast<uintptr_t>(var) << 5) | 0x22000000))

static constexpr uint32_t MAIN_VIEW = 0;

static constexpr int16_t SCREEN_WIDTH = 128;
static constexpr int16_t SCREEN_HEIGHT = 64;
static constexpr int16_t EYE_DISTANCE = 150;
static constexpr int16_t HORIZON = 15;

static float g_offset_x = 0.0f;
static float g_offset_y = 0.0f;
static int16_t g_rotation = 0;

static int16_t g_scale_x = 16;
static int16_t g_scale_y = 16;

static uint8_t g_current_background_id = 0;
static Pbm* g_current_background_pbm;

static const char* g_backgrounds[] = {
    APP_ASSETS_PATH("floor.pbm"),
    APP_ASSETS_PATH("grid.pbm"),
    APP_ASSETS_PATH("cookie_monster.pbm"),
    EXT_PATH("mode7_demo/background.pbm")};

static uint8_t* screen_buffer;

static void reload_background() {
    if(g_current_background_pbm != nullptr) {
        pbm_free(g_current_background_pbm);
    }

    Storage* storage = static_cast<Storage*>(furi_record_open(RECORD_STORAGE));

    g_current_background_pbm = pbm_load_file(storage, g_backgrounds[g_current_background_id]);
    if(g_current_background_pbm == nullptr) {
        // Try the other backgrounds, give up if none of them work (this shouldn't be the case ever)
        const uint8_t last_background_id = g_current_background_id;
        do {
            g_current_background_id = (g_current_background_id + 1) % std::size(g_backgrounds);
            if(g_current_background_id == last_background_id) {
                furi_crash();
            }
            g_current_background_pbm =
                pbm_load_file(storage, g_backgrounds[g_current_background_id]);
        } while(g_current_background_pbm == nullptr);
    }
    furi_record_close(RECORD_STORAGE);
}

static void load_scales() {
    Storage* storage = static_cast<Storage*>(furi_record_open(RECORD_STORAGE));
    Stream* file_stream = file_stream_alloc(storage);
    if(file_stream_open(
           file_stream, EXT_PATH("mode7_demo/scales.txt"), FSAM_READ, FSOM_OPEN_EXISTING)) {
        FuriString* line_string = furi_string_alloc();
        if(stream_read_line(file_stream, line_string)) {
            g_scale_x = 1;
            g_scale_y = 1;
            sscanf(
                furi_string_get_cstr(line_string), "%" SCNi16 " %" SCNi16, &g_scale_x, &g_scale_y);
        }
        furi_string_free(line_string);
    }
    file_stream_close(file_stream);

    stream_free(file_stream);
    furi_record_close(RECORD_STORAGE);
}

static int mod(int x, int divisor) {
    int m = x % divisor;
    return m + ((m >> 31) & divisor);
}

static uint32_t sample_background(
    const uint32_t* bitmap,
    int32_t sample_x,
    int32_t sample_y,
    uint32_t pitch,
    int32_t width,
    int32_t height) {
    // Repeat mode for now
    sample_x = mod(sample_x, width);
    sample_y = mod(sample_y, height);

    return bitmap[sample_y * pitch + sample_x];
}

static void handle_inputs(const InputState* input) {
    // TODO: This should react to events and cache the input buttons state, not this
    float x = 0.0f, y = 0.0f;

    if((input->held & GameKeyRight) != 0) {
        x++;
    } else if((input->held & GameKeyLeft) != 0) {
        x--;
    }
    if((input->held & GameKeyDown) != 0) {
        y++;
    } else if((input->held & GameKeyUp) != 0) {
        y--;
    }

    float angle_sin, angle_cos;
    sincosf((g_rotation * M_PI) / 180.0f, &angle_sin, &angle_cos);
    g_offset_x += x * angle_cos - y * angle_sin;
    g_offset_y += x * angle_sin + y * angle_cos;

    if((input->held & GameKeyOk) != 0) {
        g_rotation = (g_rotation + 1) % 360;
    }

    if((input->pressed & GameKeyBack) != 0) {
        g_current_background_id = (g_current_background_id + 1) % std::size(g_backgrounds);
        reload_background();
    }
}

static void frame_cb(GameEngine* engine, Canvas* canvas, InputState input, void* context) {
    UNUSED(engine);
    UNUSED(canvas);
    UNUSED(context);

    handle_inputs(&input);

    float angle_sin, angle_cos;
    sincosf((g_rotation * M_PI) / 180.0f, &angle_sin, &angle_cos);

    // "Rasterize" the background
    const int32_t background_width = g_current_background_pbm->width;
    const uint32_t background_pitch = pbm_get_pitch(g_current_background_pbm);
    const int32_t background_height = g_current_background_pbm->height;
    const uint32_t* background_bitmap = BIT_BAND_ALIAS(g_current_background_pbm->bitmap);

    // This is "slow" but simulates how backgrounds are rasterized. Use it for now.
    // This method also allows for easy repeat modes
    uint32_t* screen_bitmap = BIT_BAND_ALIAS(screen_buffer);
    for(int32_t y = -HORIZON + 1; y < SCREEN_HEIGHT / 2; ++y) {
        for(int32_t x = -SCREEN_WIDTH / 2; x < SCREEN_WIDTH / 2; ++x) {
            int32_t dx = x + (SCREEN_WIDTH / 2);
            int32_t dy = y + (SCREEN_HEIGHT / 2);

            int32_t px = x;
            int32_t py = y + EYE_DISTANCE;
            int32_t pz = y + HORIZON;

            float sx = static_cast<float>(px) / pz;
            float sy = static_cast<float>(-py) / pz;
            float rsx = sx * angle_cos - sy * angle_sin;
            float rsy = sx * angle_sin + sy * angle_cos;

            screen_bitmap[dy * 128 + dx] = sample_background(
                background_bitmap,
                (rsx * g_scale_x) + g_offset_x,
                (rsy * g_scale_y) + g_offset_y,
                background_pitch,
                background_width,
                background_height);
        }
    }

    canvas_draw_xbm(canvas, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, screen_buffer);
}

extern "C" int32_t mode7_demo_app(void* p) {
    UNUSED(p);

    reload_background();
    load_scales();

    GameEngineSettings settings = game_engine_settings_init();
    settings.target_fps = 60;
    settings.show_fps = true;
    settings.always_backlight = true;
    settings.frame_callback = frame_cb;
    settings.context = NULL;

    screen_buffer = static_cast<uint8_t*>(malloc(16 * 64));
    GameEngine* engine = game_engine_alloc(settings);

    game_engine_run(engine);
    game_engine_free(engine);
    free(screen_buffer);

    return 0;
}
