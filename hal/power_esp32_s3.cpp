/**
 * @file power_esp32_s3.cpp
 * @brief Power HAL implementation for Waveshare ESP32-S3 Touch AMOLED 1.8"
 *
 * Uses the AXP2101 PMU chip over I2C (shared bus with touch/display on
 * SDA=15, SCL=14). Wire.begin() is already called by the display/touch HAL
 * before hal_power_init(), so we only attach to the existing bus.
 *
 * See features/hal_spec_power.md for contract specification.
 */

#include "power.h"
#include <Arduino.h>
#include <Wire.h>

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

static XPowersPMU pmu;
static bool g_pmu_ready = false;

bool hal_power_init(void) {
    // Wire is already started by display/touch HAL (SDA=15, SCL=14).
    // Just attach the PMU to the existing bus.
    g_pmu_ready = pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, 15, 14);

    if (!g_pmu_ready) {
        Serial.println("[PowerHAL] AXP2101 not found on I2C bus");
        return true;  // Non-fatal: system runs fine without battery monitoring
    }

    // Enable internal ADC measurements
    pmu.enableBattDetection();
    pmu.enableBattVoltageMeasure();
    pmu.enableVbusVoltageMeasure();
    pmu.enableSystemVoltageMeasure();

    Serial.println("[PowerHAL] AXP2101 initialized");
    return true;
}

hal_power_status_t hal_power_get_status(void) {
    if (!g_pmu_ready) {
        return HAL_POWER_STATUS_UNKNOWN;
    }

    if (!pmu.isBatteryConnect()) {
        return HAL_POWER_STATUS_NO_BATTERY;
    }

    if (pmu.isCharging()) {
        return HAL_POWER_STATUS_CHARGING;
    }

    // Check if fully charged (USB in but not actively charging)
    uint8_t charger = pmu.getChargerStatus();
    if (charger == XPOWERS_AXP2101_CHG_DONE_STATE) {
        return HAL_POWER_STATUS_CHARGED;
    }

    return HAL_POWER_STATUS_DISCHARGING;
}

int8_t hal_power_get_charge_level(void) {
    if (!g_pmu_ready || !pmu.isBatteryConnect()) {
        return -1;
    }

    int percent = pmu.getBatteryPercent();
    if (percent < 0) return -1;
    if (percent > 100) percent = 100;
    return (int8_t)percent;
}

uint16_t hal_power_get_voltage_mv(void) {
    if (!g_pmu_ready || !pmu.isBatteryConnect()) {
        return 0;
    }
    return pmu.getBattVoltage();
}
