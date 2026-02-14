/**
 * @file power_manager.cpp
 * @brief Power Manager SystemComponent Implementation
 *
 * Polls hal_power at a fixed 2-second interval. The elapsed timer starts
 * at POLL_INTERVAL_S so the first update() triggers an immediate poll.
 */

#include "power_manager.h"
#include "../../hal/power.h"

PowerManager::PowerManager()
    : m_elapsed(POLL_INTERVAL_S)  // Triggers immediate poll on first update
{
}

bool PowerManager::begin() {
    return hal_power_init();
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
