/**
 * @file battery_status.h
 * @brief Battery Status Data Model
 *
 * Stores the last-known-good power telemetry. Inherits from DataItem
 * for uniform metadata (name, modification timestamp).
 *
 * Specification: features/sys_battery_metering.md §2.2
 * Architecture:  features/arch_power_management.md §2
 */

#ifndef BATTERY_STATUS_H
#define BATTERY_STATUS_H

#include "data_item.h"
#include "../../hal/power.h"

class BatteryStatus : public DataItem {
public:
    BatteryStatus()
        : DataItem("BatteryStatus")
        , m_status(HAL_POWER_STATUS_UNKNOWN)
        , m_chargeLevel(-1)
        , m_voltageMv(0) {}

    hal_power_status_t getStatus() const { return m_status; }
    int8_t getChargeLevel() const { return m_chargeLevel; }
    uint16_t getVoltageMv() const { return m_voltageMv; }

    /**
     * @brief Updates all battery fields and touches the timestamp
     */
    void update(hal_power_status_t status, int8_t chargeLevel, uint16_t voltageMv) {
        m_status = status;
        m_chargeLevel = chargeLevel;
        m_voltageMv = voltageMv;
        touch();
    }

private:
    hal_power_status_t m_status;
    int8_t m_chargeLevel;
    uint16_t m_voltageMv;
};

#endif // BATTERY_STATUS_H
