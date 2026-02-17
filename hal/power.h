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
 * @brief Power button event enumeration
 */
typedef enum {
    HAL_POWER_EVENT_NONE,          ///< No button event
    HAL_POWER_EVENT_SHORT_PRESS,   ///< Short press detected (< 1s)
    HAL_POWER_EVENT_LONG_PRESS,    ///< Long press detected (>= 4s)
} hal_power_button_event_t;

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

/**
 * @brief Initializes the power button (GPIO or PMU interrupt)
 *
 * Configures the hardware button used for suspend/resume/shutdown.
 * On AXP2101 boards: enables PEK (Power Enable Key) IRQs.
 * On SY6970 boards: configures GPIO 0 as input with pull-up.
 */
void hal_power_button_init(void);

/**
 * @brief Polls for a debounced/processed button event
 *
 * Returns the most recent button event since last call. Events are
 * consumed on read (subsequent calls return NONE until a new event).
 *
 * @return hal_power_button_event_t The detected button event
 */
hal_power_button_event_t hal_power_button_get_event(void);

/**
 * @brief Puts the CPU into low-power suspend mode
 *
 * The caller must turn off peripherals (display, WiFi, touch) before
 * calling this function. This function blocks until wakeup occurs.
 */
void hal_power_suspend(void);

/**
 * @brief Restores hardware state after waking from suspend
 *
 * Called after hal_power_suspend() returns. Re-initializes any
 * power-related hardware that was affected by suspend.
 */
void hal_power_resume(void);

/**
 * @brief Commands the PMU to cut all power rails (full shutdown)
 *
 * This function does not return. The device will power off completely.
 */
void hal_power_shutdown(void);

/**
 * @brief Gates startup after a deep sleep wakeup
 *
 * On boards that use deep sleep for shutdown (T-Display S3 Plus), this
 * verifies the user is performing an intentional startup by requiring a
 * sustained button hold. If the hold is too short, the system re-enters
 * deep sleep (does not return).
 *
 * Returns immediately on cold boot or boards with hardware PEK (Waveshare).
 * Must be called very early in setup(), before any peripheral initialization.
 */
void hal_power_check_wakeup(void);

#ifdef __cplusplus
}
#endif

#endif // HAL_POWER_H
