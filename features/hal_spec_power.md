# Feature: HAL Power Contract

> Label: "HAL Power"
> Category: "Hardware Layer"
> Prerequisite: features/hal_core_contract.md

## 1. Introduction
The Power HAL provides a hardware-independent interface for monitoring the system's power source, battery status, and charge level. It abstracts various hardware implementations, including PMU chips (e.g., AXP2101) and simple ADC-based voltage dividers.

## 2. API Definition (hal/power.h)

### 2.1 Types
```c
typedef enum {
    HAL_POWER_STATUS_UNKNOWN,
    HAL_POWER_STATUS_NO_BATTERY,   // Powered by USB/External, no battery detected
    HAL_POWER_STATUS_DISCHARGING,  // Running on battery
    HAL_POWER_STATUS_CHARGING,     // Battery is charging
    HAL_POWER_STATUS_CHARGED       // Battery is full and still connected to power
} hal_power_status_t;
```

### 2.2 Functions
- `bool hal_power_init(void)`: Initializes the power monitoring hardware.
- `hal_power_status_t hal_power_get_status(void)`: Returns the current power/battery state.
- `int8_t hal_power_get_charge_level(void)`: Returns charge level as a percentage (0-100), or -1 if unavailable/no battery.
- `uint16_t hal_power_get_voltage_mv(void)`: Returns battery voltage in millivolts.

## 3. Implementation Requirements

### 3.1 Stub Implementation (`hal/power_stub.cpp`)
- Must provide a predictable mock behavior for native testing.
- Default to `HAL_POWER_STATUS_DISCHARGING` at 75% for testing.

### 3.2 Target Implementations
- **ESP32-S3 (Waveshare/LilyGo):** 
    - If a PMU (AXP2101) is present, use its registers for accurate status and percentage.
    - If only an ADC is present, calculate percentage based on a standard LiPo discharge curve (e.g., 3.27V = 0%, 4.2V = 100%).
    - Detection of "NO BATTERY" should be handled via PMU flags or ADC voltage being consistently near 0V or Vbus-drop level.

## 4. Scenarios

### Scenario: USB Power with No Battery
- Given the device is plugged into USB
- And no battery is physically connected
- Then `hal_power_get_status()` returns `HAL_POWER_STATUS_NO_BATTERY`
- And `hal_power_get_charge_level()` returns -1.

### Scenario: Charging Battery
- Given the device is plugged into USB
- And a battery is charging
- Then `hal_power_get_status()` returns `HAL_POWER_STATUS_CHARGING`.

### Scenario: Discharging Battery
- Given the device is NOT plugged into USB
- And is running on battery
- Then `hal_power_get_status()` returns `HAL_POWER_STATUS_DISCHARGING`
- And `hal_power_get_charge_level()` returns a value between 0 and 100.

## Implementation Notes

### [2026-02-14] ADC Pin Configuration
Both `power_esp32_s3.cpp` and `power_tdisplay_s3_plus.cpp` default to GPIO 4 with a 2:1 voltage divider. These values MUST be verified during HIL testing for each board. The LiPo discharge curve uses 3270mV=0% and 4200mV=100%. Charging detection uses >4250mV threshold. `NO_BATTERY` triggers below 100mV.
