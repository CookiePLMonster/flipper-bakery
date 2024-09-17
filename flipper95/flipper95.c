#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>

#include <stdint.h>

#include <mbedtls/bignum.h>

#define TAG "Flipper95"

typedef struct {
    FuriPubSub* input;
    FuriPubSubSubscription* input_subscription;
    Gui* gui;
    Canvas* canvas;
    bool stop;
} Flipper95;

static void gui_input_events_callback(const void* value, void* ctx) {
    furi_assert(value);
    furi_assert(ctx);

    Flipper95* instance = ctx;
    const InputEvent* event = value;

    if(event->key == InputKeyBack && event->type == InputTypeShort) {
        instance->stop = true;
    }
}

static Flipper95* flipper95_alloc(void) {
    Flipper95* instance = malloc(sizeof(Flipper95));

    instance->input = furi_record_open(RECORD_INPUT_EVENTS);
    instance->gui = furi_record_open(RECORD_GUI);
    instance->canvas = gui_direct_draw_acquire(instance->gui);

    instance->input_subscription =
        furi_pubsub_subscribe(instance->input, gui_input_events_callback, instance);

    return instance;
}

static void flipper95_free(Flipper95* instance) {
    furi_pubsub_unsubscribe(instance->input, instance->input_subscription);

    gui_direct_draw_release(instance->gui);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_INPUT_EVENTS);
}

bool trial_division_is_prime(uint32_t n) {
    uint32_t sqrt_n = (unsigned int)sqrt((double)n);
    uint32_t x;
    if((n % 2) == 0) return 2;
    for(x = 3; x <= sqrt_n; x += 2) {
        if((n % x) == 0) {
            return false;
        }
    }
    return true;
}

static void flipper95_run(Flipper95* instance) {
    furi_thread_set_current_priority(FuriThreadPriorityIdle);

    canvas_reset(instance->canvas);

    uint32_t p = 2;
    mbedtls_mpi M_p, S, temp;
    mbedtls_mpi_init(&M_p);
    mbedtls_mpi_init(&S);
    mbedtls_mpi_init(&temp);

    size_t buffer_len = 256;
    char* buffer = malloc(buffer_len);

    do {
        // Take the next prime p
        do {
            ++p;
        } while(!trial_division_is_prime(p));

        mbedtls_mpi_lset(&M_p, 1);
        mbedtls_mpi_shift_l(&M_p, p); // 2^p
        mbedtls_mpi_sub_int(&M_p, &M_p, 1); // 2^p - 1

        // Perform the LL test
        mbedtls_mpi_lset(&S, 4);
        for(uint32_t i = 0; i < p - 2; i++) {
            // S = (S^2 - 2) % M_p
            mbedtls_mpi_mul_mpi(&temp, &S, &S); // temp = S^2
            mbedtls_mpi_sub_int(&temp, &temp, 2); // temp = S^2 - 2
            mbedtls_mpi_mod_mpi(&S, &temp, &M_p); // S = (S^2 - 2) % M_p
        }

        if(mbedtls_mpi_cmp_int(&S, 0) == 0) {
            canvas_clear(instance->canvas);

            canvas_set_font(instance->canvas, FontSecondary);
            canvas_draw_str_aligned(
                instance->canvas, 1, 0, AlignLeft, AlignTop, "Last prime found:");
            snprintf(buffer, buffer_len, "M%lu", p);
            canvas_draw_str_aligned(instance->canvas, 126, 0, AlignRight, AlignTop, buffer);

            size_t required_buffer_len = 0;
            mbedtls_mpi_write_string(&M_p, 10, buffer, 0, &required_buffer_len);
            if(required_buffer_len > buffer_len) {
                free(buffer);
                buffer = malloc(required_buffer_len);
                buffer_len = required_buffer_len;
            }
            if(mbedtls_mpi_write_string(&M_p, 10, buffer, buffer_len, &required_buffer_len) == 0) {
                canvas_set_font(instance->canvas, FontKeyboard);
                const size_t LINE_LENGTH = 21;
                const size_t MAX_LENGTH = 7 * LINE_LENGTH;

                // Split the text into LINE_LENGTH characters each.
                // We can fit only 7 lines, so if the string is longer than that, add ellipsis and terminate it early
                if(required_buffer_len >= MAX_LENGTH) {
                    strncpy(buffer + MAX_LENGTH - 3, "...", 4);
                }
                const char* buffer_ptr = buffer;
                int32_t y = 8;
                for(size_t i = 0; i < required_buffer_len && i < MAX_LENGTH; i += LINE_LENGTH) {
                    char temp_buf[LINE_LENGTH + 1];
                    strlcpy(temp_buf, buffer_ptr, sizeof(temp_buf));

                    canvas_draw_str_aligned(instance->canvas, 1, y, AlignLeft, AlignTop, temp_buf);
                    y += 8;

                    buffer_ptr += LINE_LENGTH;
                }
            }
            canvas_commit(instance->canvas);
        }

        furi_thread_yield();
    } while(!instance->stop);

    free(buffer);

    mbedtls_mpi_free(&M_p);
    mbedtls_mpi_free(&S);
    mbedtls_mpi_free(&temp);
}

int32_t flipper95_app(void* p) {
    UNUSED(p);

    Flipper95* instance = flipper95_alloc();
    flipper95_run(instance);
    flipper95_free(instance);

    return 0;
}
