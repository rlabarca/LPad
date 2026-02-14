/**
 * @file power_tdisplay_s3_plus.cpp
 * @brief Power HAL implementation for LilyGo T-Display S3 Plus AMOLED
 *
 * SAFE PASSTHROUGH: Returns NO_BATTERY until the correct ADC pin is
 * identified during HIL testing. GPIO 4 was found to conflict with
 * PSRAM/Flash data lines on the ESP32-S3 WROOM module, causing a
 * watchdog-reset boot loop.
 *
 * TODO(HIL): Identify the battery voltage ADC pin from the board
 * schematic, then enable analogRead() here.
 *
 * See features/hal_spec_power.md for contract specification.
 */

#include "power.h"
#include <Arduino.h>

// ==========================================================
// ADC configuration — DISABLED until HIL pin verification
// ==========================================================
// static const uint8_t BATTERY_ADC_PIN = ???;  // TBD from schematic
// static const float VOLTAGE_DIVIDER_RATIO = 2.0f;

// LiPo discharge curve thresholds (millivolts) — ready for use
// once the ADC pin is known.
static const uint16_t LIPO_MIN_MV = 3270;
static const uint16_t LIPO_MAX_MV = 4200;
static const uint16_t NO_BATTERY_THRESHOLD_MV = 100;
static const uint16_t CHARGING_THRESHOLD_MV = 4250;

bool hal_power_init(void) {
    // No GPIO configuration until correct pin is known
    Serial.println("[PowerHAL] tdisplay_s3_plus: safe mode (no ADC pin configured)");
    return true;
}

uint16_t hal_power_get_voltage_mv(void) {
    // ADC disabled — return 0 (triggers NO_BATTERY path)
    return 0;
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
