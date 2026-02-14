/**
 * @file power_manager.h
 * @brief Power Manager SystemComponent (Z=0) — Background Battery Polling
 *
 * Polls the Power HAL every 2 seconds and stores results in a BatteryStatus
 * data object. Registered at Z=0 (runs before all UI components).
 * Does not render anything — purely a background polling service.
 *
 * Specification: features/sys_battery_metering.md §2.1
 * Architecture:  features/arch_power_management.md
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "../ui/ui_component.h"
#include "../data/battery_status.h"

class PowerManager : public SystemComponent {
public:
    PowerManager();

    /**
     * @brief Initializes the Power HAL
     * @return true if HAL init succeeded
     */
    bool begin();

    /**
     * @brief Accessor for the current battery telemetry
     * @return Pointer to the internal BatteryStatus (always valid)
     */
    const BatteryStatus* getBatteryStatus() const { return &m_battery; }

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

    void pollHardware();
};

#endif // POWER_MANAGER_H
