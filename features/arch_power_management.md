# Architectural Policy: Power Management & Telemetry

> Label: "Power Policy"
> Category: "ARCHITECTURES"
> Prerequisite: features/hal_spec_power.md
> Prerequisite: features/arch_data_strategy.md

## 1. Decentralized Polling
Power status is telemetry data, not an event-driven system (unless IRQs are specifically required for emergency shutdown).
- The system MUST poll the Power HAL at a fixed interval.
- **Interval:** 2000ms (2 seconds).
- Polling MUST NOT block the UI rendering thread for more than 5ms.

## 2. Global Power State (Data Layer)
The current power state MUST be stored in a system-global `BatteryStatus` data structure that inherits from `DataItem`.
- This ensures the UI and other background apps can read the "Last Known Good" power state without triggering a slow I2C/ADC read every frame.

## 3. Visual Consistency
The display of power status across different UI layers (System Menu, Status Bar, Apps) MUST follow these color/naming conventions:
- **NO BATTERY:** Color = `colors.text_status` (same as WiFi).
- **CHARGING:** Color = Green (`0x07E0` or similar).
- **DISCHARGING / CHARGED:** Color = `colors.text_status`.
- **LOW BATTERY (< 15%):** Color = Red (`0xF800`).

## 4. Lifecycle Integration
Power monitoring should start as early as possible in `setup()` but after the `Timer HAL` is initialized, as it depends on `hal_timer_get_micros()` via `DataItem`.
