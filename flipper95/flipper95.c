#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <cli/cli.h>
#include <toolbox/args.h>
#include <toolbox/strint.h>

#include <furi_hal_power.h>

#include <stdatomic.h>

#include <mbedtls/bignum.h>

#define CLI_COMMAND "flipper95"

typedef struct {
    FuriPubSub* input;
    FuriPubSubSubscription* input_subscription;
    Gui* gui;
    Canvas* canvas;
    Cli* cli;

    // These are protected by the mutex
    FuriMutex* state_mutex;
    uint32_t cur_mnumber; // Current Mersenne number analyzed
    uint32_t cur_mprime; // Last Mersenne prime found
    char* mprime_str;
    size_t mprime_str_len;

    atomic_bool stop;
} Flipper95;

static void gui_input_events_callback(const void* value, void* ctx) {
    furi_assert(value);
    furi_assert(ctx);

    Flipper95* instance = ctx;
    const InputEvent* event = value;

    if(event->key == InputKeyBack && event->type == InputTypeShort) {
        atomic_store_explicit(&instance->stop, true, memory_order_relaxed);
    }
}

static bool thread_signal_callback(uint32_t signal, void* arg, void* context) {
    UNUSED(arg);
    if(signal == FuriSignalExit) {
        Flipper95* instance = context;
        atomic_store_explicit(&instance->stop, true, memory_order_relaxed);
        return true;
    }
    return false;
}

static void flipper95_cli_print_usage() {
    printf("Stress test your Flipper by calculating Mersenne primes, same as Prime95 does\r\n"
           "\r\n"
           "Usage:\r\n" CLI_COMMAND " <cmd> <args>\r\n"
           "Cmd list:\r\n"
           "\tadvance M<p:int> - Advance calculations to a Mersenne number M_p\r\n"
           "\tprint\t\t - Print the last found Mersenne prime\r\n");
}

static bool flipper95_cli_set_mnumber(const FuriString* args, Flipper95* instance) {
    // TODO: furi_string_start_withi when it's available
    if(furi_string_size(args) > 2 && furi_string_start_with(args, "M")) {
        uint32_t number;
        if(strint_to_uint32(furi_string_get_cstr(args) + 1, NULL, &number, 10) ==
           StrintParseNoError) {
            furi_check(furi_mutex_acquire(instance->state_mutex, FuriWaitForever) == FuriStatusOk);
            instance->cur_mnumber = number;
            furi_check(furi_mutex_release(instance->state_mutex) == FuriStatusOk);
            return true;
        }
    }
    return false;
}

static void flipper95_cli_callback(Cli* cli, FuriString* args, void* context) {
    UNUSED(cli);
    Flipper95* instance = context;

    FuriString* cmd = furi_string_alloc();
    bool success = false;
    if(args_read_string_and_trim(args, cmd)) {
        if(furi_string_equal(cmd, "advance")) {
            success = flipper95_cli_set_mnumber(args, instance);
        }
    }

    if(!success) {
        flipper95_cli_print_usage();
    }
}

static void flipper95_init(Flipper95* instance) {
    instance->input = furi_record_open(RECORD_INPUT_EVENTS);
    instance->gui = furi_record_open(RECORD_GUI);
    instance->canvas = gui_direct_draw_acquire(instance->gui);
    instance->cli = furi_record_open(RECORD_CLI);

    instance->input_subscription =
        furi_pubsub_subscribe(instance->input, gui_input_events_callback, instance);

    instance->state_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    instance->cur_mnumber = 520;
    instance->cur_mprime = 0;
    instance->mprime_str_len = 256;
    instance->mprime_str = malloc(instance->mprime_str_len);
    instance->mprime_str[0] = '\0';

    atomic_init(&instance->stop, false);
}

static void flipper95_deinit(Flipper95* instance) {
    free(instance->mprime_str);
    furi_mutex_free(instance->state_mutex);

    furi_pubsub_unsubscribe(instance->input, instance->input_subscription);

    furi_record_close(RECORD_CLI);
    gui_direct_draw_release(instance->gui);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_INPUT_EVENTS);
}

bool trial_division_is_prime(uint32_t n) {
    if(n == 2) return true;
    if((n % 2) == 0) return false;
    const uint32_t sqrt_n = (unsigned int)sqrt((double)n);
    for(uint32_t x = 3; x <= sqrt_n; x += 2) {
        if((n % x) == 0) {
            return false;
        }
    }
    return true;
}

static void canvas_draw_ascii_str_wrapped_ellipsis(
    Canvas* canvas,
    const int32_t x,
    const int32_t y,
    const int32_t wrap_x,
    const int32_t max_y,
    const char* str) {
    const int32_t font_height = 8;
    int32_t cur_x = x;
    int32_t cur_y = y;

    bool overflow = false;
    const int32_t ellipsis_width = canvas_string_width(canvas, "...");
    while(*str != '\0') {
        const int32_t glyph_width = canvas_glyph_width(canvas, *str);
        const bool last_line = cur_y + font_height > max_y;
        const int32_t max_width_for_line = !last_line ? wrap_x : wrap_x - ellipsis_width;
        if(cur_x + glyph_width > max_width_for_line) {
            if(last_line) {
                overflow = true;
                break;
            }
            cur_x = x;
            cur_y += font_height;
        }
        canvas_draw_glyph(canvas, cur_x, cur_y, *str);
        cur_x += glyph_width;
        str++;
    }

    if(overflow) {
        canvas_draw_str(canvas, cur_x, cur_y, "...");
    }
}

static void flipper95_run(Flipper95* instance) {
    furi_thread_set_current_priority(FuriThreadPriorityIdle);
    furi_thread_set_signal_callback(furi_thread_get_current(), thread_signal_callback, instance);
    cli_add_command(
        instance->cli, CLI_COMMAND, CliCommandFlagParallelSafe, flipper95_cli_callback, instance);
    furi_hal_power_insomnia_enter();

    canvas_reset(instance->canvas);
    canvas_set_font(instance->canvas, FontSecondary);

    mbedtls_mpi M_p, S, temp;
    mbedtls_mpi_init(&M_p);
    mbedtls_mpi_init(&S);
    mbedtls_mpi_init(&temp);

    do {
        // Current Mersenne number may have been advanced by the user via CLI when we weren't looking
        furi_check(furi_mutex_acquire(instance->state_mutex, FuriWaitForever) == FuriStatusOk);
        uint32_t p = instance->cur_mnumber;
        furi_check(furi_mutex_release(instance->state_mutex) == FuriStatusOk);
        const uint32_t last_number =
            p; // We'll use this later to check if the user has advanced the calculations themselves

        // Take the next prime p
        while(!trial_division_is_prime(p)) {
            ++p;
        }

        // Print the canvas BEFORE starting the iteration, so the current number is updated before
        // we start heavy calculations.
        char buffer[32];
        snprintf(buffer, sizeof(buffer), ">M%lu", p);
        canvas_draw_str_aligned(instance->canvas, 128, 0, AlignRight, AlignTop, buffer);

        canvas_commit(instance->canvas);
        canvas_clear(instance->canvas);

        mbedtls_mpi_lset(&M_p, 1);
        mbedtls_mpi_shift_l(&M_p, p); // 2^p
        mbedtls_mpi_sub_int(&M_p, &M_p, 1); // 2^p - 1

        // Perform the LL test
        mbedtls_mpi_lset(&S, 4);
        for(uint32_t i = 0;
            i < p - 2 && !atomic_load_explicit(&instance->stop, memory_order_relaxed);
            i++) {
            // S = (S^2 - 2) % M_p
            mbedtls_mpi_mul_mpi(&temp, &S, &S); // temp = S^2
            mbedtls_mpi_sub_int(&temp, &temp, 2); // temp = S^2 - 2
            mbedtls_mpi_mod_mpi(&S, &temp, &M_p); // S = (S^2 - 2) % M_p
        }

        // Now update all the values we usually read from under the lock.
        // We can render the Mersenne prime without a lock, as this is the only place where it can update.
        const bool is_prime = mbedtls_mpi_cmp_int(&S, 0) == 0;
        furi_check(furi_mutex_acquire(instance->state_mutex, FuriWaitForever) == FuriStatusOk);
        if(instance->cur_mnumber == last_number) {
            instance->cur_mnumber = p + 1;
        }
        if(is_prime) {
            instance->cur_mprime = p;

            size_t required_buffer_len = 0;
            mbedtls_mpi_write_string(
                &M_p, 10, instance->mprime_str, instance->mprime_str_len, &required_buffer_len);
            if(required_buffer_len > instance->mprime_str_len) {
                free(instance->mprime_str);
                instance->mprime_str = malloc(required_buffer_len);
                instance->mprime_str_len = required_buffer_len;
            }
            furi_check(
                mbedtls_mpi_write_string(
                    &M_p,
                    10,
                    instance->mprime_str,
                    instance->mprime_str_len,
                    &required_buffer_len) == 0);
        }
        furi_check(furi_mutex_release(instance->state_mutex) == FuriStatusOk);

        snprintf(buffer, sizeof(buffer), "Last prime: M%lu", instance->cur_mprime);
        canvas_draw_str_aligned(instance->canvas, 0, 0, AlignLeft, AlignTop, buffer);

        // Render the prime one character at a time, with line breaks and ellipsis
        canvas_draw_ascii_str_wrapped_ellipsis(
            instance->canvas, 0, 16, 128, 64, instance->mprime_str);

        furi_thread_yield();
    } while(!atomic_load_explicit(&instance->stop, memory_order_relaxed));

    mbedtls_mpi_free(&M_p);
    mbedtls_mpi_free(&S);
    mbedtls_mpi_free(&temp);

    furi_hal_power_insomnia_exit();
    cli_delete_command(instance->cli, CLI_COMMAND);
}

int32_t flipper95_app(void* p) {
    UNUSED(p);

    Flipper95 instance;
    flipper95_init(&instance);
    flipper95_run(&instance);
    flipper95_deinit(&instance);

    return 0;
}
