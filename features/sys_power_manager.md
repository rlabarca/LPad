# Feature: Power Manager

> Label: "System: Power Manager"
> Category: "System Components"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/arch_concurrency.md
> Prerequisite: features/arch_hal_contract.md

[TODO]

## 1. Overview

The PowerManager is a SystemComponent (Z=0) responsible for battery telemetry polling, power button event handling, and orchestrating the full suspend/resume/shutdown lifecycle. It polls the power HAL every 2 seconds for battery status and processes short press (suspend) and long press (shutdown) button events. The suspend sequence carefully orders peripheral shutdown to prevent LWIP corruption and I2C state issues. Battery telemetry is exposed via the BatteryStatus data model.

---

## 2. Requirements

### 2.1 Battery Polling

- MUST poll `hal_power_get_status()`, `hal_power_get_charge_level()`, and `hal_power_get_voltage_mv()` every 2.0 seconds (POLL_INTERVAL_S).
- Results are stored in a `BatteryStatus` data object accessible via `getBatteryStatus()`.
- First poll happens immediately on construction (elapsed initialized to POLL_INTERVAL_S).

### 2.2 Button Handling

- `handle()` MUST be called before `renderAll()` each frame.
- SHORT_PRESS triggers `suspend()`.
- LONG_PRESS triggers `shutdown()`.
- NONE is ignored.

### 2.3 Suspend Sequence

- Order: (1) set state SUSPENDED, (2) pause active app (stops background tasks), (3) display sleep, (4) network disconnect, (5) touch sleep, (6) HAL suspend (blocks until wake).
- The active app MUST be paused before network disconnect to prevent mid-flight HTTP requests from corrupting LWIP state.

### 2.4 Resume Sequence

- Order: (1) HAL resume, (2) network reconnect, (3) poll until CONNECTED or 10s timeout, (4) display wake, (5) touch wake, (6) unpause active app, (7) set state RUNNING, (8) force immediate battery poll.
- WiFi reconnect blocks with 250ms poll interval to ensure stable network before app resumes.
- Active app unpause forces full redraw (AMOLED GRAM undefined after sleep).

### 2.5 Shutdown Sequence

- Order: (1) display sleep, (2) network disconnect, (3) touch sleep, (4) HAL shutdown (no return).

### 2.6 Component Properties

- `render()` is empty (PowerManager has no visual output).
- `isOpaque()` returns `false`. `isFullscreen()` returns `false`.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Battery status polled every 2 seconds

    Given the PowerManager is running
    When 2.0 seconds of update(dt) accumulate
    Then hal_power_get_status, get_charge_level, and get_voltage_mv are called
    And getBatteryStatus reflects the new values

#### Scenario: Short press triggers suspend

    Given hal_power_button_get_event returns SHORT_PRESS
    When handle() is called
    Then the suspend sequence executes

#### Scenario: Long press triggers shutdown

    Given hal_power_button_get_event returns LONG_PRESS
    When handle() is called
    Then the shutdown sequence executes

#### Scenario: Active app paused before network disconnect on suspend

    Given the StockTickerApp is the active app
    When suspend is called
    Then onPause is called on the app before hal_network_disconnect

#### Scenario: Resume waits for WiFi with timeout

    Given the device resumes from suspend
    When hal_network_reconnect is called
    Then the resume sequence polls network status every 250ms
    And proceeds after CONNECTED or 10 seconds

#### Scenario: Immediate battery poll after resume

    Given the device has just resumed
    When the first update(dt) runs
    Then a battery poll executes immediately (no 2s wait)

### Manual Scenarios (Human Verification Required)

#### Scenario: Suspend and resume cycle preserves state

    Given the device is displaying a stock chart
    When the power button is short-pressed
    Then the screen turns off
    And pressing the power button again restores the chart display
