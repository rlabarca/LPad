/**
 * @file power.h
 * @brief Hardware Abstraction Layer (HAL) - Power Monitoring Contract
 *
 * This header defines the abstract interface for monitoring the system's
 * power source, battery status, and charge level. Abstracts PMU chips
 * (e.g., AXP2101) and simple ADC-based voltage dividers.
 *
 * See features/hal_spec_power.md for complete specification.
 */

#ifndef HAL_POWER_H
#define HAL_POWER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power/battery status enumeration
 */
typedef enum {
    HAL_POWER_STATUS_UNKNOWN,      ///< Status not yet determined
    HAL_POWER_STATUS_NO_BATTERY,   ///< Powered by USB/External, no battery detected
    HAL_POWER_STATUS_DISCHARGING,  ///< Running on battery
    HAL_POWER_STATUS_CHARGING,     ///< Battery is charging
    HAL_POWER_STATUS_CHARGED       ///< Battery is full and still connected to power
} hal_power_status_t;

/**
 * @brief Initializes the power monitoring hardware
 *
 * Initializes ADC, PMU, or other power monitoring peripherals.
 * Must be called once before any other power functions.
 *
 * @return true if initialization was successful, false otherwise
 */
bool hal_power_init(void);

/**
 * @brief Returns the current power/battery state
 *
 * @return hal_power_status_t Current power status
 */
hal_power_status_t hal_power_get_status(void);

/**
 * @brief Returns the battery charge level as a percentage
 *
 * @return Charge level 0-100, or -1 if unavailable/no battery
 */
int8_t hal_power_get_charge_level(void);

/**
 * @brief Returns the battery voltage in millivolts
 *
 * @return Battery voltage in mV, or 0 if unavailable
 */
uint16_t hal_power_get_voltage_mv(void);

#ifdef __cplusplus
}
#endif

#endif // HAL_POWER_H
