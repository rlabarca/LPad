/**
 * @file power_manager.h
 * @brief Power Manager SystemComponent (Z=0) — Battery Polling & Power States
 *
 * Polls the Power HAL every 2 seconds and stores results in a BatteryStatus
 * data object. Also manages system power states (suspend, resume, shutdown)
 * by polling the power button and orchestrating peripheral sleep/wake.
 *
 * Specification: features/sys_battery_metering.md §2.1
 *                features/sys_power_states.md
 * Architecture:  features/arch_power_management.md
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "../ui/ui_component.h"
#include "../data/battery_status.h"

/**
 * @brief System power state
 */
enum class PowerState {
    RUNNING,    ///< Normal operation
    SUSPENDED,  ///< Low-power suspend (display off, WiFi off)
};

class PowerManager : public SystemComponent {
public:
    PowerManager();

    /**
     * @brief Initializes the Power HAL and button detection
     * @return true if HAL init succeeded
     */
    bool begin();

    /**
     * @brief Polls for button events and handles power state transitions
     *
     * Must be called every iteration of the main loop, BEFORE
     * UIRenderManager::renderAll(). When a suspend is triggered,
     * this function blocks until resume.
     */
    void handle();

    /**
     * @brief Accessor for the current battery telemetry
     * @return Pointer to the internal BatteryStatus (always valid)
     */
    const BatteryStatus* getBatteryStatus() const { return &m_battery; }

    /**
     * @brief Returns the current power state
     */
    PowerState getState() const { return m_state; }

    // SystemComponent overrides
    void update(float dt) override;
    void render() override {}

    bool isOpaque() const override { return false; }
    bool isFullscreen() const override { return false; }

    /// Polling interval in seconds (per arch_power_management.md §1)
    static constexpr float POLL_INTERVAL_S = 2.0f;

private:
    BatteryStatus m_battery;
    float m_elapsed;
    PowerState m_state;

    void pollHardware();
    void suspend();
    void resume();
    void shutdown();
};

#endif // POWER_MANAGER_H
