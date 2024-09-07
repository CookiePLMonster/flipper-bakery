#include "digital_clock_app.hpp"

#include "scenes/scenes.hpp"

#include <cookie/common>

#include <stm32wbxx_ll_lptim.h>
#include <stm32wbxx_ll_rcc.h>
#include <stm32wbxx_ll_rtc.h>

#include <furi/core/event_loop.h>
#include <furi_hal_bus.h>
#include <furi_hal_interrupt.h>

using namespace cookie;

#define TICK_TOCK_TIMER LPTIM2
static constexpr auto TICK_TOCK_TIMER_BUS = FuriHalBusLPTIM2;
static constexpr auto TICK_TOCK_TIMER_INTERRUPT = FuriHalInterruptIdLpTim2;
static constexpr auto TICK_TOCK_TIMER_IRQ = LPTIM2_IRQn;
static constexpr uint32_t TICK_TOCK_TIMER_FREQ = 32768u;

DigitalClockApp::DigitalClockApp()
    : m_scene_manager(scene_handlers, this)
    , m_time_sync_thread(
          "TimeSync",
          1024,
          [](void* context) {
              DigitalClockApp* view = reinterpret_cast<DigitalClockApp*>(context);
              return view->TimeSyncThread();
          },
          this) {
    // TODO: Consider introducing cookie::FuriEventLoopSubscriptionCookie that takes pointers
    // to subscribed elements and thus can auto-unsubscribe from the destructor at zero cost (empty class)
    // Best to wait for that when ViewDispatcher can adopt an event loop, it'll simplify semantics
    // (can use a pointer to cookie::FuriEventLoop in a template)
    furi_event_loop_subscribe_semaphore(
        view_dispatcher_get_event_loop(*m_view_dispatcher),
        *m_tick_tock_semaphore,
        FuriEventLoopEventIn,
        [](FuriEventLoopObject* object, void* context) {
            DigitalClockApp* app = reinterpret_cast<DigitalClockApp*>(context);
            app->SendAppEvent(AppFlowEvent::TickTock);
            furi_semaphore_acquire(reinterpret_cast<::FuriSemaphore*>(object), FuriWaitForever);
            return true;
        },
        this);

    view_dispatcher_attach_to_gui(*m_view_dispatcher, *m_gui, ViewDispatcherTypeFullscreen);

    view_dispatcher_set_event_callback_context(*m_view_dispatcher, this);
    view_dispatcher_set_custom_event_callback(*m_view_dispatcher, &CustomEventCallback);
    view_dispatcher_set_navigation_event_callback(*m_view_dispatcher, &BackEventCallback);

    // TODO: Similarly to the above, consider cookie::ViewDispatcher::ViewCookie that
    // automatically removes the view. Should again be an empty class.
    view_dispatcher_add_view(
        *m_view_dispatcher, FuriEnumParam(AppView::Init), *m_init_view.View());
    view_dispatcher_add_view(
        *m_view_dispatcher, FuriEnumParam(AppView::Clock), *m_clock_view.View());

    furi_hal_bus_enable(TICK_TOCK_TIMER_BUS);
    furi_hal_interrupt_set_isr(
        TICK_TOCK_TIMER_INTERRUPT,
        [](void* context) {
            LL_LPTIM_ClearFlag_ARRM(TICK_TOCK_TIMER);

            DigitalClockApp* app = reinterpret_cast<DigitalClockApp*>(context);
            furi_semaphore_release(*app->m_tick_tock_semaphore);
        },
        this);

    LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM2_CLKSOURCE_LSE);

    LL_LPTIM_InitTypeDef tickTockTimerDef{};
    tickTockTimerDef.Prescaler = LL_LPTIM_PRESCALER_DIV128;
    LL_LPTIM_Init(TICK_TOCK_TIMER, &tickTockTimerDef);
}

DigitalClockApp::~DigitalClockApp() {
    StopTickTockTimer();

    furi_hal_interrupt_set_isr(TICK_TOCK_TIMER_INTERRUPT, nullptr, nullptr);
    furi_hal_bus_disable(TICK_TOCK_TIMER_BUS);

    view_dispatcher_remove_view(*m_view_dispatcher, FuriEnumParam(AppView::Clock));
    view_dispatcher_remove_view(*m_view_dispatcher, FuriEnumParam(AppView::Init));

    StopTimeSync();
    furi_thread_join(*m_time_sync_thread);

    furi_event_loop_unsubscribe(
        view_dispatcher_get_event_loop(*m_view_dispatcher), *m_tick_tock_semaphore);
}

void DigitalClockApp::Run() {
    scene_manager_next_scene(*m_scene_manager, FuriEnumParam(AppScene::Init));
    view_dispatcher_run(*m_view_dispatcher);
}

void DigitalClockApp::Exit() {
    scene_manager_stop(*m_scene_manager);
    view_dispatcher_stop(*m_view_dispatcher);
}

void DigitalClockApp::SwitchToView(AppView view) const {
    view_dispatcher_switch_to_view(*m_view_dispatcher, FuriEnumParam(view));
}

void DigitalClockApp::SendAppEvent(AppFlowEvent event) const {
    view_dispatcher_send_custom_event(*m_view_dispatcher, FuriEnumParam(event));
}

bool DigitalClockApp::SearchAndSwitchToAnotherScene(AppScene scene) const {
    return scene_manager_search_and_switch_to_another_scene(
        *m_scene_manager, FuriEnumParam(scene));
}

void DigitalClockApp::NextScene(AppScene scene) const {
    scene_manager_next_scene(*m_scene_manager, FuriEnumParam(scene));
}

void DigitalClockApp::StartTimeSync() {
    furi_thread_set_priority(*m_time_sync_thread, FuriThreadPriorityIdle);
    furi_thread_start(*m_time_sync_thread);
}

void DigitalClockApp::StopTimeSync() {
    furi_thread_flags_set(
        furi_thread_get_id(*m_time_sync_thread), FuriEnumParam(SyncThreadEvent::Shutdown));
}

void DigitalClockApp::StartTickTockTimer() {
    if(m_tick_tock_timer_running) {
        return;
    }
    m_tick_tock_timer_running = true;

    LL_LPTIM_Enable(TICK_TOCK_TIMER);
    while(!LL_LPTIM_IsEnabled(TICK_TOCK_TIMER))
        ;

    LL_LPTIM_EnableIT_ARRM(TICK_TOCK_TIMER);
    LL_LPTIM_SetAutoReload(TICK_TOCK_TIMER, 127); // Autoreload twice a second
    LL_LPTIM_StartCounter(TICK_TOCK_TIMER, LL_LPTIM_OPERATING_MODE_CONTINUOUS);
}

void DigitalClockApp::StopTickTockTimer() {
    if(!m_tick_tock_timer_running) {
        return;
    }
    m_tick_tock_timer_running = false;

    furi_hal_bus_reset(TICK_TOCK_TIMER_BUS);
    NVIC_ClearPendingIRQ(TICK_TOCK_TIMER_IRQ);
}

bool DigitalClockApp::CustomEventCallback(void* context, uint32_t event) {
    DigitalClockApp* app = reinterpret_cast<DigitalClockApp*>(context);
    return scene_manager_handle_custom_event(*app->m_scene_manager, event);
}

bool DigitalClockApp::BackEventCallback(void* context) {
    DigitalClockApp* app = reinterpret_cast<DigitalClockApp*>(context);
    return scene_manager_handle_back_event(*app->m_scene_manager);
}

int32_t DigitalClockApp::TimeSyncThread() {
    auto GetSeconds = [] {
        ScopedFuriCritical critical;
        uint32_t seconds = LL_RTC_TIME_GetSecond(RTC);
        const uint32_t date = LL_RTC_DATE_Get(RTC);
        UNUSED(date); // This read is necessary to unlock the shadow registers
        return __LL_RTC_CONVERT_BCD2BIN(seconds);
    };

    // We need subseconds to be available, wait for the shift operation to end before starting the sync
    {
        ScopedFuriCritical critical;
        while(LL_RTC_IsActiveFlag_SHP(RTC)) {
            furi_thread_yield();
        }
    }

    const uint32_t start_seconds = GetSeconds();
    const uint32_t target_seconds = (start_seconds + 3) % 60;

    // Spin the thread until the next second, then order to switch to the next scene ASAP
    while(true) {
        uint32_t events =
            furi_thread_flags_wait(FuriEnumParam(SyncThreadEvent::Mask), FuriFlagWaitAny, 0);

        if((events & FuriFlagError) == 0 &&
           (events & FuriEnumParam(SyncThreadEvent::Shutdown)) != 0) {
            break;
        }

        const uint32_t seconds = GetSeconds();
        if(seconds == target_seconds) {
            SendAppEvent(AppFlowEvent::SyncDone);

            // Switch away ASAP to let the scene switch
            furi_thread_yield();
            break;
        }

        furi_thread_yield();
    }

    return 0;
}
