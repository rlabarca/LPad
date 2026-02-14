/**
 * @file power_tdisplay_s3_plus.cpp
 * @brief Power HAL implementation for LilyGo T-Display S3 Plus AMOLED
 *
 * Uses ADC-based battery voltage reading. Pin configuration and voltage
 * divider ratio should be verified during HIL testing.
 *
 * See features/hal_spec_power.md for contract specification.
 */

#include "power.h"
#include <Arduino.h>

// ADC pin for battery voltage (via voltage divider)
// NOTE: Verify this pin during HIL testing for the LilyGo board
static const uint8_t BATTERY_ADC_PIN = 4;

// Voltage divider ratio (Vbat = Vadc * ratio)
static const float VOLTAGE_DIVIDER_RATIO = 2.0f;

// LiPo discharge curve thresholds (millivolts)
static const uint16_t LIPO_MIN_MV = 3270;
static const uint16_t LIPO_MAX_MV = 4200;
static const uint16_t NO_BATTERY_THRESHOLD_MV = 100;
static const uint16_t CHARGING_THRESHOLD_MV = 4250;

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
