> Prerequisite: features/hal_spec_display.md

# Feature: Display Baseline for ESP32-S3-Touch-AMOLED

> Label: "ESP32-S3 AMOLED Driver"
> Category: "Hardware Layer"

This feature describes the work required to implement the Display HAL contract for the ESP32-S3-Touch-AMOLED target hardware.

## 1. Target Hardware & Vendor Code Reference

*   **Target Hardware:** ESP32-S3-Touch-AMOLED-1.8
*   **Vendor Code Reference:** The implementation MUST be a port of the logic found in the vendor-provided examples.
    *   **Path:** `hw-examples/ESP32-S3-Touch-AMOLED-1.8-Demo/`
    *   **Focus:** Analyze the `.ino` files and any associated libraries (`.cpp`, `.h`) within the `Arduino-v3.3.5` sub-directory to understand the display initialization sequence, pin configurations, and drawing commands.

## 2. Implementation Plan

The agent (Claude) will create a new HAL implementation file, `hal/display_esp32_s3_amoled.cpp`, that implements the functions defined in `features/hal_spec_display.md`.

## 3. Scenarios

**Scenario: Successful Display Initialization**
*   **Given:** The `hal_display_init` function is implemented in `hal/display_esp32_s3_amoled.cpp` by porting the initialization logic from the vendor reference code.
*   **When:** The `hal_display_init` function is called from a test application.
*   **Then:** The function should return `true`.
*   **And:** The display backlight should turn on. (Visual confirmation step for user)

**Scenario: Clear Display to a Solid Color**
*   **Given:** The display has been successfully initialized.
*   **And:** The `hal_display_clear` function is implemented by porting the relevant vendor drawing commands.
*   **When:** The `hal_display_clear` function is called with the color `0x001F` (pure blue).
*   **Then:** The driver commands for clearing the screen should execute without errors.
*   **And:** The entire display should turn solid blue. (Visual confirmation step for user)

**Scenario: Draw a Single Pixel**
*   **Given:** The display has been successfully initialized and cleared.
*   **And:** The `hal_display_draw_pixel` and `hal_display_flush` functions are implemented by porting vendor commands.
*   **When:** `hal_display_draw_pixel(100, 100, 0xFFFF)` is called.
*   **And:** `hal_display_flush()` is called.
*   **Then:** The driver commands for drawing a pixel and flushing the buffer should execute without errors.
*   **And:** A single white pixel should be visible at coordinates (100, 100). (Visual confirmation step for user)

## Implementation Notes

### [2026-02-11] Brightness 255 — No PWM Flicker
The Waveshare ESP32-S3 1.8" AMOLED uses the same RM67162 panel family. Brightness must be set to 255 to avoid PWM flicker (same issue as T-Display S3 Plus). See `features/display_tdisplay_s3_plus.md` for details.

### [2026-02-11] No TE Sync Required
Unlike the T-Display S3 Plus, this board does not expose a TE (Tearing Effect) pin. The QSPI bus is fast enough that tearing is minimal without explicit sync. If tearing is observed in future, consider adding a software vsync delay.

### [2026-02-08] QSPI Bus Configuration
This board uses a QSPI (quad SPI) bus for display communication, configured via `Arduino_ESP32QSPI`. The pin mapping differs from standard SPI and must match the vendor reference in `hw-examples/ESP32-S3-Touch-AMOLED-1.8-Demo/`.
