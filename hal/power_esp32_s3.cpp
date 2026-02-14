/**
 * @file power_esp32_s3.cpp
 * @brief Power HAL implementation for Waveshare ESP32-S3 Touch AMOLED 1.8"
 *
 * Uses ADC-based battery voltage reading with a standard LiPo discharge curve.
 * Pin configuration and voltage divider ratio should be verified during HIL testing.
 *
 * See features/hal_spec_power.md for contract specification.
 */

#include "power.h"
#include <Arduino.h>

// ADC pin for battery voltage (via voltage divider)
// NOTE: Verify this pin during HIL testing for the Waveshare board
static const uint8_t BATTERY_ADC_PIN = 4;

// Voltage divider ratio (Vbat = Vadc * ratio)
// Typical 2:1 divider (100K/100K) → ratio = 2.0
static const float VOLTAGE_DIVIDER_RATIO = 2.0f;

// LiPo discharge curve thresholds (millivolts)
static const uint16_t LIPO_MIN_MV = 3270;   // 0% — cutoff voltage
static const uint16_t LIPO_MAX_MV = 4200;   // 100% — fully charged
static const uint16_t NO_BATTERY_THRESHOLD_MV = 100;
static const uint16_t CHARGING_THRESHOLD_MV = 4250;  // Above max → charging

bool hal_power_init(void) {
    analogSetAttenuation(ADC_11db);
    pinMode(BATTERY_ADC_PIN, INPUT);
    return true;
}

uint16_t hal_power_get_voltage_mv(void) {
    uint32_t raw_mv = analogReadMilliVolts(BATTERY_ADC_PIN);
    uint16_t battery_mv = (uint16_t)(raw_mv * VOLTAGE_DIVIDER_RATIO);
    return battery_mv;
}

hal_power_status_t hal_power_get_status(void) {
    uint16_t mv = hal_power_get_voltage_mv();

    if (mv < NO_BATTERY_THRESHOLD_MV) {
        return HAL_POWER_STATUS_NO_BATTERY;
    }
    if (mv >= CHARGING_THRESHOLD_MV) {
        return HAL_POWER_STATUS_CHARGING;
    }
    if (mv >= LIPO_MAX_MV) {
        return HAL_POWER_STATUS_CHARGED;
    }
    return HAL_POWER_STATUS_DISCHARGING;
}

int8_t hal_power_get_charge_level(void) {
    uint16_t mv = hal_power_get_voltage_mv();

    if (mv < NO_BATTERY_THRESHOLD_MV) {
        return -1;
    }

    int32_t level = ((int32_t)mv - LIPO_MIN_MV) * 100 / (LIPO_MAX_MV - LIPO_MIN_MV);
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    return (int8_t)level;
}
