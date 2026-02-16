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

### [2026-02-14] MUST Use Callback-Based I2C Init (Shared Bus)
XPowersLib's `pmu.begin(Wire, addr, sda, scl)` calls `Wire.begin(sda, scl)` internally (XPowersCommon.tpp line 160), which reinitializes the I2C bus already running for the touch controller (FT3168). **Fix:** Use the callback-based `pmu.begin(addr, readCb, writeCb)` which skips all Wire management and uses the already-initialized bus via custom read/write functions. Read/write callbacks include 3-retry logic with 200µs inter-attempt delay to handle intermittent NACK from bus contention with the FT3168 touch controller.

### [2026-02-14] Voltage-Based % Over Coulomb Counter
`getBatteryPercent()` reads the AXP2101 coulomb counter register which defaults to 100% on fresh init and requires a full charge/discharge cycle to calibrate. Use `getBattVoltage()` with LiPo curve (3270mV=0%, 4200mV=100%) for immediate accuracy.

### [2026-02-14] Charging % Rate Limiter (Terminal Voltage Spike)
During active charging, the battery terminal voltage is elevated above OCV by charge current × internal resistance + electrochemical overpotential. This causes the voltage-based % to spike to ~100% instantly on charger connect. Fix: cache last reported level (`g_last_level`) and cap increases to +1% per poll (2s intervals) when `isCharging()`. On charger disconnect, `isCharging()` returns false so the rate limiter disengages and the OCV-based level reads accurately. On boot, `g_last_level` starts at -1 (no cache), so the first reading uses raw voltage (accurate OCV before charging starts).

### [2026-02-14] T-Display S3 Plus Still Safe Passthrough
The LilyGo `power_tdisplay_s3_plus.cpp` remains a safe passthrough (returns NO_BATTERY) until the correct power monitoring method is identified for that board during HIL.

### [2026-02-15] Post-Unplug Surface Charge Causes % Creep
When the USB charger is physically disconnected, the battery terminal voltage does NOT immediately drop to OCV (open-circuit voltage). Surface charge on the cell keeps voltage elevated for several seconds. If `g_last_level` resets or upward movement is allowed while not charging, the elevated readings cause the displayed % to creep upward after unplug. **Fix:** Split the rate-limiter into two modes: (1) **Charging:** slow ramp +1% per 5 polls (10s) to smooth charger voltage spikes. (2) **Not charging:** freeze — never allow upward movement. A battery that isn't charging cannot gain charge; any upward voltage reading is surface charge settling or ADC noise. On the T-Display (BQ25896/SY6970), also removed the `g_last_level = -1` reset on charge→discharge transition to prevent accepting the first elevated post-unplug reading. Observed on both ESP32-S3 Waveshare and T-Display S3 Plus boards.

### [2026-02-15] T-Display: Phantom NO_BATTERY During Active Charging
The BQ25896/SY6970 charge current register (0x12) has 50mA step granularity, and the I2C bus is shared with the touch controller. Bus contention can cause `readChargeCurrentRaw()` to fail all 3 retries and return 0, which trips the `ichg < MIN_REAL_CHARGE_MA` phantom battery check — falsely displaying NO_BATTERY for one poll cycle during active charging. **Fix:** Debounce with a consecutive-readings counter (`g_no_batt_count`). Require 3 consecutive low-current readings before declaring NO_BATTERY. Single glitches return CHARGING instead. Counter resets to 0 on any successful current reading >= 50mA. The ESP32-S3 Waveshare does not need this fix because the AXP2101 PMU uses `isBatteryConnect()` (a dedicated flag) rather than charge current inference.

### [2026-02-15] Battery Level Still Somewhat Unreliable (Known Limitation)
After all rate-limiter and debounce fixes, the displayed battery percentage is functional but still not fully reliable. Voltage-based percentage is inherently approximate — it depends on load, temperature, cell age, and the gap between terminal voltage and true OCV. The current implementation is "good enough" for user-facing display but should not be treated as precise. Future improvements could include coulomb counting calibration or longer-window averaging.
