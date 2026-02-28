# Implementation Notes: HAL Timer

### Source Mapping

| File | Role |
|---|---|
| `hal/timer.h` | C interface contract |
| `hal/timer_esp32.cpp` | ESP32 `esp_timer_get_time()` implementation |
| `hal/timer_stub.cpp` | Host-native test stub (weak-linked) |
| `test/test_timer_hal/test_timer_hal.cpp` | Unit tests |

### Stub Design

Both stub functions are declared `__attribute__((weak))` so test fixtures can override them by defining the same symbols without the weak attribute. Default stub returns `false` / `0`.

### ESP32 Implementation

`hal_timer_init()` is a no-op on ESP32 because `esp_timer` is initialized by the FreeRTOS startup sequence before `app_main()` runs. Returning `true` unconditionally is correct.

### [AUTONOMOUS] Weak Stub Returns false, Not true

The spec requires `hal_timer_init()` to return `true` on success. The ESP32 implementation does. The stub returns `false` by design (not an error — it signals no real timer is present). Tests that exercise the stub expect `false`; tests that override the stub can return any value. This dual-meaning is consistent with the spec's "stub" intent.
