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

- Create a new file: `hal/timer_esp32.cpp`.
- This file should include `<hal/display.h>` to bring in the function declarations from the contract, and `<esp_timer.h>` for the ESP-IDF timer functions.
- The `hal_timer_init()` function should be implemented, although for `esp_timer`, no explicit initialization is needed, so it can simply return `true`.
- The `hal_timer_get_micros()` function must be implemented as a simple wrapper around `esp_timer_get_time()`.
- Update `hal/display.h` to include the function declarations for the Timer HAL API if they are not already present, guarded by an include guard.
- Add a new `hal/timer_stub.cpp` file that provides a weak, empty implementation for non-ESP32 builds to prevent linker errors.
- Update `platformio.ini` to exclude `hal/timer_stub.cpp` from the ESP32 build environments and exclude `hal/timer_esp32.cpp` from non-ESP32 environments.

### `hal/display.h` additions
```cpp
// Add to the bottom of hal/display.h, inside the include guard
#include <cstdint>

// Timer HAL Contract
bool hal_timer_init(void);
uint64_t hal_timer_get_micros(void);
```

### `hal/timer_esp32.cpp`
```cpp
#include <hal/display.h> // This should contain the timer function declarations
#include <esp_timer.h>

bool hal_timer_init(void) {
    // esp_timer is initialized by default, no action needed
    return true;
}

uint64_t hal_timer_get_micros(void) {
    return esp_timer_get_time();
}
```

### `hal/timer_stub.cpp`
```cpp
#include <hal/display.h>

// Provide a weak stub implementation for non-ESP32 builds to avoid linker errors.
// The build system should be configured to exclude this file for ESP32 targets.
__attribute__((weak)) bool hal_timer_init(void) {
    return false;
}

__attribute__((weak)) uint64_t hal_timer_get_micros(void) {
    return 0;
}
```

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
