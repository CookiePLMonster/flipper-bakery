#pragma once

#include <cookie/coroutine>
#include <cookie/timer>
#include <cookie/within>
#include <cookie/gui/view>
#include <cookie/gui/view_dispatcher>

class DigitalClockApp;

class DigitalClockView : cookie::Within<DigitalClockView, DigitalClockApp>,
                         cookie::EnableWithin<DigitalClockView> {
public:
    DigitalClockView(FuriEventLoop* event_loop);

    const auto& View() const {
        return m_view;
    }

private:
    struct Model {
        Model(bool twelve_hour_clock)
            : twelve_hour_clock(twelve_hour_clock) {
        }

        uint8_t hour, minute, second;
        uint8_t tenths_of_second;

        bool lock_screen_shown;

        const bool twelve_hour_clock;
    };

    void OnEnter();
    void OnExit();
    void OnTimeUpdate();
    bool OnInput(const InputEvent* event);
    static void OnDraw(Canvas* canvas, const Model& model);

private:
    void ShowLockScreen(bool show);

    outer_type* get_outer() const;

private:
    cookie::ViewModel<Model> m_view;

    const uint32_t PREDIV_S;

    // Lock screen view overlay
    // TODO: May need to be separated to a different view and used in more places,
    // but for now keep it here.
    class LockScreenOverlay : cookie::Within<LockScreenOverlay, DigitalClockView> {
    public:
        LockScreenOverlay(FuriEventLoop* event_loop);

        void OnExit();

        // Returns true if input was consumed
        bool OnInput(const InputEvent* event);
        static void OnDraw(Canvas* canvas, const Model& model);

        cookie::Task<bool> ProcessExitInputsAsync();

    private:
        outer_type* get_outer() const;

    private:
        cookie::AwaitableTimer<cookie::FuriEventLoopTimer> m_lock_timer;
        cookie::Task<bool> m_lock_task;
    };
    LockScreenOverlay m_lock_overlay;

private:
    DEFINE_GET_FROM_INNER(m_lock_overlay);
};
