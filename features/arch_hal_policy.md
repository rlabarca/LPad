# Architectural Policy: HAL Barrier

> Label: "HAL Policy"
> Category: "ARCHITECTURES"
> Prerequisite: None

## 1. Strict Separation
Application code (`src/*.cpp`) **MUST NOT** include hardware-specific headers (e.g., `esp_lcd_panel_io.h`) or vendor libraries directly. It must **ONLY** include domain-specific HAL headers (e.g., `hal/display.h`, `hal/network.h`).

## 2. No `Arduino.h` in Logic
Application logic should minimize reliance on `Arduino.h`. Where used (e.g., `setup()`, `loop()`, or basic types), it must not couple logic to specific board capabilities.

## 3. The Stub Pattern
Every HAL contract **MUST** have a corresponding `_stub.cpp` implementation. This ensures the application can always compile and run (logic-only) on any platform, even without specific hardware support (e.g., CI/CD pipelines, native testing).

## 4. Implementation Patterns
*   **DMA Mandate:** All display drivers **MUST** use DMA (Direct Memory Access) for pixel transfers where hardware supports it.
*   **Blocking vs. Non-Blocking:** Drawing commands are generally blocking until data is *sent* to the DMA buffer, but return before the transfer is complete on the wire. `hal_display_flush()` acts as the synchronization barrier.
*   **Feature Flagging:** Hardware targeting is controlled entirely by build flags in `platformio.ini`. Code uses `#ifdef` guards based on these flags to select the active HAL implementation at compile time.
