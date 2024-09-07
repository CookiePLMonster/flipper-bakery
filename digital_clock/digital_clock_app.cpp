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

DigitalClockApp::DigitalClockApp() {
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
            app->SendAppEvent(AppLogicEvent::TickTock);
            furi_semaphore_acquire(reinterpret_cast<::FuriSemaphore*>(object), FuriWaitForever);
            return true;
        },
        this);

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

void DigitalClockApp::SendAppEvent(AppLogicEvent event) const {
    view_dispatcher_send_custom_event(*m_view_dispatcher, FuriEnumParam(event));
}

bool DigitalClockApp::SearchAndSwitchToAnotherScene(AppScene scene) const {
    return scene_manager_search_and_switch_to_another_scene(
        *m_scene_manager, FuriEnumParam(scene));
}

void DigitalClockApp::NextScene(AppScene scene) const {
    scene_manager_next_scene(*m_scene_manager, FuriEnumParam(scene));
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

::FuriEventLoop* DigitalClockApp::GetEventLoop() const {
    return view_dispatcher_get_event_loop(*m_view_dispatcher);
}

bool DigitalClockApp::CustomEventCallback(void* context, uint32_t event) {
    DigitalClockApp* app = reinterpret_cast<DigitalClockApp*>(context);
    return scene_manager_handle_custom_event(*app->m_scene_manager, event);
}

bool DigitalClockApp::BackEventCallback(void* context) {
    DigitalClockApp* app = reinterpret_cast<DigitalClockApp*>(context);
    return scene_manager_handle_back_event(*app->m_scene_manager);
}
