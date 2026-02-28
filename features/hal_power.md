# Feature: HAL Power

> Label: "HAL: Power"
> Category: "HAL"
> Prerequisite: features/arch_hal_contract.md
> Prerequisite: features/arch_concurrency.md

[TODO]

## 1. Overview

The Power HAL provides a hardware-agnostic C interface for battery monitoring, power button event detection, and system power state management (suspend/resume/shutdown) on LPad devices. It abstracts PMU differences (AXP2101 on Waveshare, BQ25896/SY6970 on LilyGo T-Display S3 Plus) and includes sophisticated workarounds for library bugs, phantom battery detection, I2C bus contention, and charge level rate-limiting. The battery telemetry data model (charge percentage, voltage, status) is integrated into this feature.

---

## 2. Requirements

### 2.1 Lifecycle

- `hal_power_init()` MUST be non-fatal: it returns `true` even if the PMU chip is not found, because the system must remain operational without battery monitoring.
- `hal_power_init()` MUST be idempotent.
- `hal_power_check_wakeup()` MUST be called very early in `setup()` before peripheral initialization. On SY6970 boards, if the wakeup cause is EXT0 and the button is not held for 2 seconds, it MUST re-enter deep sleep.

### 2.2 Battery Monitoring

- `hal_power_get_status()` MUST return one of: UNKNOWN, NO_BATTERY, DISCHARGING, CHARGING, CHARGED.
- `hal_power_get_charge_level()` MUST return 0-100 (percentage) or -1 if no battery or chip not found.
- `hal_power_get_voltage_mv()` MUST return battery voltage in millivolts or 0 if unavailable.
- Battery percentage MUST use a LiPo voltage curve: 3270mV = 0%, 4200mV = 100%, below 3000mV = no battery.

### 2.3 Charge Level Rate-Limiting

- While charging: upward jumps are limited to +1% per 5 polls (10-second smoothing window).
- While discharging: charge level MUST NOT increase (surface charge and measurement noise are suppressed by freezing the level).
- This prevents the charge percentage from jumping erratically.

### 2.4 Phantom Battery Detection

- On SY6970 boards, VBUS leakage can cause the chip to report a battery when none is connected.
- Phantom battery MUST be detected by: pre-charge mode with charge current < 50mA indicates no real battery.
- Detection MUST require 3 consecutive confirming reads to debounce I2C contention false positives.

### 2.5 Power Button Events

- `hal_power_button_get_event()` MUST return SHORT_PRESS (< 1000ms), LONG_PRESS (>= 2000ms SY6970 / >= 4s AXP2101 hardware), or NONE.
- Events are consume-on-read: subsequent calls return NONE until the next event.
- AXP2101 uses hardware IRQ via I2C; SY6970 uses GPIO 0 software state machine.
- On wake from suspend, the button state MUST be set to WAIT_RELEASE to avoid immediate re-trigger.

### 2.6 Suspend

- `hal_power_suspend()` blocks until the device is woken.
- The caller MUST have already turned off the display, WiFi, and touch before calling suspend.
- SY6970 board: MUST call `esp_wifi_stop()` before light sleep (WiFi timer callbacks reference flash, which is inaccessible during sleep). After wake, MUST call `Wire.end()` + `Wire.begin()` to reinitialize the I2C peripheral.
- AXP2101 board: Uses a CPU polling loop (100ms intervals) because the PMU IRQ goes through an I2C GPIO expander (XCA9554), not a direct GPIO.

### 2.7 Shutdown

- `hal_power_shutdown()` does not return.
- AXP2101 board: calls `pmu.shutdown()` which cuts all power rails.
- SY6970 board: uses `esp_deep_sleep_start()` with GPIO 0 ext0 wakeup. MUST NOT use `disableBATFET()` (irreversible -- bricks the device until USB connected).
- SY6970 board: MUST wait for GPIO 0 to go HIGH (button released) before entering deep sleep. ext0 wakeup is level-triggered: if the button is held, the device immediately reboots.

### 2.8 Stub Requirements

- The stub MUST compile on host-native without embedded SDK dependencies.
- Default state: DISCHARGING at 75% charge, 3800mV.
- Test helpers: `hal_power_stub_set_status()`, `hal_power_stub_set_charge_level()`, `hal_power_stub_set_voltage_mv()`.
- `hal_power_button_get_event()` always returns NONE.
- Suspend, resume, shutdown, check_wakeup are all no-ops.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Power init is non-fatal without PMU

    Given no PMU chip is present on the I2C bus
    When hal_power_init is called
    Then it returns true
    And subsequent charge level queries return -1

#### Scenario: Charge level maps voltage to percentage

    Given the battery voltage is 3735mV (midpoint of 3270-4200 range)
    When hal_power_get_charge_level is called
    Then it returns approximately 50

#### Scenario: Below minimum voltage reports no battery

    Given the battery voltage is below 3000mV
    When hal_power_get_status is called
    Then it returns HAL_POWER_STATUS_NO_BATTERY

#### Scenario: Charge level rate-limited while charging

    Given the device is charging
    And the previous charge level was 50%
    When the raw voltage suddenly indicates 80%
    Then hal_power_get_charge_level increases by at most 1% per 5 polls

#### Scenario: Charge level frozen while discharging

    Given the device is discharging
    And the previous charge level was 70%
    When the raw voltage briefly indicates 72% (noise)
    Then hal_power_get_charge_level remains at or below 70%

#### Scenario: Button event is consume-on-read

    Given a short press event has occurred
    When hal_power_button_get_event is called
    Then it returns HAL_POWER_EVENT_SHORT_PRESS
    And a second immediate call returns HAL_POWER_EVENT_NONE

#### Scenario: Stub default state

    Given the native_test stub is active
    When hal_power_get_status is called
    Then it returns HAL_POWER_STATUS_DISCHARGING
    And hal_power_get_charge_level returns 75
    And hal_power_get_voltage_mv returns 3800

#### Scenario: Stub test helper overrides charge level

    Given the native_test stub is active
    When hal_power_stub_set_charge_level(42) is called
    Then hal_power_get_charge_level returns 42

### Manual Scenarios (Human Verification Required)

#### Scenario: Battery percentage tracks correctly on Waveshare

    Given the Waveshare device is running on battery
    When the battery discharges over 30 minutes
    Then the reported charge level decreases smoothly without erratic jumps

#### Scenario: Suspend and resume cycle on T-Display S3 Plus

    Given the T-Display S3 Plus device is running
    When the power button is short-pressed
    Then the display turns off and the device enters light sleep
    And pressing the power button again wakes the device
    And the display shows the previous screen content

#### Scenario: Shutdown on T-Display S3 Plus

    Given the T-Display S3 Plus device is running
    When the power button is held for 4 seconds
    Then the device shuts down completely
    And holding the power button for 2 seconds restarts it
