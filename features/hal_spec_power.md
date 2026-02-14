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

### [2026-02-14] GPIO 4 = LCD_SDIO0 (Display Data Line)
GPIO 4 on the Waveshare ESP32-S3 AMOLED 1.8" is the QSPI display data line (`LCD_SDIO0` in pin_config.h). Calling `pinMode(4, INPUT)` destroys the display bus and triggers a TG1WDT_SYS_RST boot loop.

### [2026-02-14] AXP2101 PMU via I2C (Waveshare esp32s3)
The Waveshare board uses an AXP2101 PMU at I2C address 0x34 on the shared bus (SDA=15, SCL=14). Wire is already initialized by the display/touch HAL. The PMU provides `isBatteryConnect()`, `isCharging()`, `getBattVoltage()`, `getBatteryPercent()`, and `getChargerStatus()` — no GPIO ADC needed. Init is non-fatal: if PMU is absent, functions return UNKNOWN/-1/0.

### [2026-02-14] T-Display S3 Plus Still Safe Passthrough
The LilyGo `power_tdisplay_s3_plus.cpp` remains a safe passthrough (returns NO_BATTERY) until the correct power monitoring method is identified for that board during HIL.
