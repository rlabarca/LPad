> Prerequisite: features/hal_contracts.md

# Feature: HAL Timer Implementation for ESP32

## Introduction

This feature provides a concrete implementation of the Timer HAL contract for ESP32-based hardware targets. It uses the ESP-IDF's high-resolution timer (`esp_timer`) to fulfill the contract's requirements.

## Scenario: Get Microsecond Timestamp
- **Given** the system is running on an ESP32 target
- **When** the `hal_timer_init()` function is called
- **Then** it should return `true` indicating success.
- **And** subsequent calls to `hal_timer_get_micros()`
- **Should** return a `uint64_t` value representing the number of microseconds since boot.
- **And** this value must be directly sourced from the `esp_timer_get_time()` function.

## Implementation Details

The Builder is responsible for creating the concrete implementation of the Timer HAL contract for the ESP32 platform.

1.  **Timer Contract Header:** Ensure the function declarations for `hal_timer_init()` and `hal_timer_get_micros()` are present in a shared HAL header file (e.g., `hal/display.h` or a new `hal/timer.h`), as defined in `features/hal_contracts.md`.
2.  **ESP32 Implementation (`hal/timer_esp32.cpp`):**
    *   Create a new file at `hal/timer_esp32.cpp`.
    *   Implement the `hal_timer_init()` function. Since the ESP-IDF `esp_timer` initializes automatically, this function can simply return `true`.
    *   Implement the `hal_timer_get_micros()` function. This function MUST be a thin wrapper that directly calls and returns the value from the ESP-IDF's `esp_timer_get_time()` function.
3.  **Stub Implementation (`hal/timer_stub.cpp`):**
    *   Create a new file at `hal/timer_stub.cpp`.
    *   Provide `__attribute__((weak))` stub implementations for both `hal_timer_init()` and `hal_timer_get_micros()`.
    *   This ensures that builds for non-ESP32 targets will link successfully without requiring a full timer implementation. `hal_timer_init` should return `false` and `hal_timer_get_micros` should return `0`.

## Build System Configuration (`platformio.ini`)

The `platformio.ini` file must be updated to correctly link the appropriate timer implementation for each environment.

```ini
[env:generic_esp32]
; ... existing flags
src_filter =
  +<*>
  -<hal/display_stub.cpp>
  -<hal/display_tdisplay_s3_plus.cpp>
  -<hal/timer_stub.cpp>
  +<hal/display_esp32s3_amoled.cpp>
  +<hal/timer_esp32.cpp>

[env:tdisplay_s3_plus]
; ... existing flags
src_filter =
  +<*>
  -<hal/display_stub.cpp>
  -<hal/display_esp32s3_amoled.cpp>
  -<hal/timer_stub.cpp>
  +<hal/display_tdisplay_s3_plus.cpp>
  +<hal/timer_esp32.cpp>
```

*(Note: The above `src_filter` examples assume a base configuration that includes all `src` and `hal` files and then selectively excludes the incorrect HAL implementations. The exact filter may need adjustment based on the project's `platformio.ini` structure.)*
