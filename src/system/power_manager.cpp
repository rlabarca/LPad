/**
 * @file power_manager.cpp
 * @brief Power Manager SystemComponent Implementation
 *
 * Polls hal_power at a fixed 2-second interval and manages system power
 * states (suspend/resume/shutdown) by orchestrating peripheral sleep/wake.
 *
 * Specification: features/sys_power_states.md
 */

#include "power_manager.h"
#include "../../hal/power.h"
#include "../../hal/display.h"
#include "../../hal/network.h"
#include "../../hal/touch.h"

#ifndef UNIT_TEST
#include <Arduino.h>
#else
extern "C" void delay(unsigned long ms);
#endif

PowerManager::PowerManager()
    : m_elapsed(POLL_INTERVAL_S)  // Triggers immediate poll on first update
    , m_state(PowerState::RUNNING)
{
}

bool PowerManager::begin() {
    bool ok = hal_power_init();
    hal_power_button_init();
    return ok;
}

void PowerManager::handle() {
    // Poll for button events from the HAL
    hal_power_button_event_t event = hal_power_button_get_event();

    switch (event) {
        case HAL_POWER_EVENT_SHORT_PRESS:
            if (m_state == PowerState::RUNNING) {
                suspend();
            }
            // Resume is handled inside hal_power_suspend() which blocks
            // until the wake button is pressed, then returns here.
            break;

        case HAL_POWER_EVENT_LONG_PRESS:
            shutdown();
            break;

        case HAL_POWER_EVENT_NONE:
        default:
            break;
    }
}

void PowerManager::update(float dt) {
    m_elapsed += dt;
    if (m_elapsed >= POLL_INTERVAL_S) {
        m_elapsed = 0.0f;
        pollHardware();
    }
}

void PowerManager::pollHardware() {
    hal_power_status_t status = hal_power_get_status();
    int8_t level = hal_power_get_charge_level();
    uint16_t voltage = hal_power_get_voltage_mv();
    m_battery.update(status, level, voltage);
}

void PowerManager::suspend() {
#ifndef UNIT_TEST
    Serial.println("[PowerManager] === SUSPENDING ===");
    Serial.flush();
#endif

    m_state = PowerState::SUSPENDED;

    // 1. Turn off display
    hal_display_sleep();

    // 2. Disconnect WiFi
    hal_network_disconnect();

    // 3. Put touch controller to sleep
    hal_touch_sleep();

    // 4. Enter hardware suspend (blocks until wakeup)
    hal_power_suspend();

    // --- Wakeup occurred ---
    resume();
}

void PowerManager::resume() {
#ifndef UNIT_TEST
    Serial.println("[PowerManager] === RESUMING ===");
    Serial.flush();
#endif

    // 1. Restore power HAL state (re-seed rate limiter etc.)
    hal_power_resume();

    // 2. Wake display
    hal_display_wake();

    // 3. Wake touch controller
    hal_touch_wake();

    // 4. Reconnect WiFi (asynchronous — connection happens in background)
    // The WiFi config is managed by the application layer; we just
    // signal that the network should be re-initialized. The system menu
    // and other components will pick up the new connection status.
    // Note: We don't re-init WiFi here because the SSID/password are
    // managed at the application level. The main loop or system menu
    // will handle reconnection.

    m_state = PowerState::RUNNING;

    // Force an immediate battery poll on resume
    m_elapsed = POLL_INTERVAL_S;

#ifndef UNIT_TEST
    Serial.println("[PowerManager] === RESUMED ===");
    Serial.flush();
#endif
}

void PowerManager::shutdown() {
#ifndef UNIT_TEST
    Serial.println("[PowerManager] === SHUTTING DOWN ===");
    Serial.flush();
#endif

    // 1. Turn off display
    hal_display_sleep();

    // 2. Disconnect WiFi
    hal_network_disconnect();

    // 3. Put touch to sleep
    hal_touch_sleep();

    // 4. Cut power (does not return)
    hal_power_shutdown();
}
