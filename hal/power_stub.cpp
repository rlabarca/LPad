/**
 * @file power_stub.cpp
 * @brief Stub implementation of Power HAL for native testing
 *
 * Provides predictable mock behavior for unit tests.
 * Default: DISCHARGING at 75%, 3800mV (per hal_spec_power.md §3.1).
 */

#include "power.h"

static hal_power_status_t g_stub_status = HAL_POWER_STATUS_DISCHARGING;
static int8_t g_stub_charge_level = 75;
static uint16_t g_stub_voltage_mv = 3800;

bool hal_power_init(void) {
    return true;
}

hal_power_status_t hal_power_get_status(void) {
    return g_stub_status;
}

int8_t hal_power_get_charge_level(void) {
    return g_stub_charge_level;
}

uint16_t hal_power_get_voltage_mv(void) {
    return g_stub_voltage_mv;
}

void hal_power_button_init(void) {
    // No-op in stub
}

hal_power_button_event_t hal_power_button_get_event(void) {
    return HAL_POWER_EVENT_NONE;
}

void hal_power_suspend(void) {
    // No-op in stub
}

void hal_power_resume(void) {
    // No-op in stub
}

void hal_power_shutdown(void) {
    // No-op in stub
}

void hal_power_check_wakeup(void) {
    // No-op in stub
}

// Test helper functions (not part of HAL API)
#ifdef UNIT_TEST
void hal_power_stub_set_status(hal_power_status_t status) {
    g_stub_status = status;
}

void hal_power_stub_set_charge_level(int8_t level) {
    g_stub_charge_level = level;
}

void hal_power_stub_set_voltage_mv(uint16_t mv) {
    g_stub_voltage_mv = mv;
}
#endif
