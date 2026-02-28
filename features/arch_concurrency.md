# Architecture: Concurrency

> Label: "Architecture: Concurrency"
> Category: "Architecture"

## Purpose

Defines the FreeRTOS concurrency model used by LPad firmware on the ESP32-S3 dual-core processor. Core 0 handles WiFi and background data fetching tasks; Core 1 runs the UI render loop. This separation ensures the 30 fps render loop is never blocked by network I/O, while shared data structures are protected by explicit synchronization primitives.

## Concurrency Invariants

### Core Assignment Rules

- Core 0: WiFi stack, background data fetch tasks (e.g., StockTracker), and any long-running I/O operations.
- Core 1: UI render loop (`loop()`), touch input processing, component update/render cycles.
- No task may perform blocking network I/O on Core 1.

### Data Sharing Rules

- Any data structure written by a Core 0 task and read by Core 1 MUST be protected by a FreeRTOS mutex (e.g., `xSemaphoreTake`/`xSemaphoreGive`).
- Simple cross-core boolean signals (e.g., "WiFi connected") MAY use `volatile` variables without a mutex, but only for single-writer/single-reader flag patterns.
- Mutex hold times MUST be minimized. Copy data out under the lock; process it after releasing.

### Suspend/Resume Ordering

- Before entering light sleep, the active app MUST be paused to stop background tasks (e.g., HTTP fetches). Failure to do so corrupts LWIP state when WiFi is torn down mid-flight.
- `esp_wifi_stop()` (not just `WiFi.disconnect()`) is required before light sleep because WiFi timer callbacks reference flash, which is inaccessible during sleep.
- After waking from light sleep, `Wire.end()` + `Wire.begin()` MUST be called to reinitialize the I2C peripheral, whose hardware state is lost during sleep while the Wire library's `_started` flag survives in RAM.

### I2C Bus Contention

- The I2C bus is shared between touch and power controllers. Reads from either domain may fail due to bus contention.
- Power HAL implementations MUST retry I2C reads (up to 3 attempts) before reporting failure.
- Phantom battery detection requires multi-sample debouncing to avoid false readings caused by I2C contention.

## Scenarios

No automated or manual scenarios. This is a policy anchor node -- its "scenarios" are
process invariants enforced by instruction files and tooling.
