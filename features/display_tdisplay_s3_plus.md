> Prerequisite: features/hal_spec_display.md

# Feature: Refactor Display HAL for T-Display-S3 AMOLED Plus using Arduino_GFX

> Label: "T-Display S3+ Driver"
> Category: "Hardware Layer"

This feature describes the work required to refactor the Display HAL contract for the T-Display-S3 AMOLED Plus, replacing the manual SPI implementation with one based on the `Arduino_GFX` library. This standardizes the display driver architecture and enables more advanced features like canvas-based drawing.

## 1. Implementation Strategy

*   **Target Hardware:** T-Display-S3 AMOLED Plus
*   **Driver Library:** The implementation **MUST** use the `Arduino_GFX_Library` located in `hw-examples/ESP32-S3-Touch-AMOLED-1.8-Demo/Arduino-v3.3.5/libraries/GFX_Library_for_Arduino/`.
*   **Reference Implementation:** The new implementation should be architecturally similar to `hal/display_esp32_s3_amoled.cpp`, which already uses `Arduino_GFX`.

## 2. Implementation Plan

The agent (Claude) will replace the entire contents of `hal/display_tdisplay_s3_plus.cpp` with a new implementation that uses the `Arduino_GFX` library to fulfill the `hal/display.h` contract.

## 3. Scenarios

**Scenario: Successful Display Initialization with Arduino_GFX**
*   **Given:** The `hal/display_tdisplay_s3_plus.cpp` file is rewritten.
*   **And:** The implementation correctly defines the bus and pin configurations for the T-Display S3 AMOLED Plus.
*   **And:** The `hal_display_init` function instantiates the `Arduino_ESP32SPI` for the bus and the `Arduino_RM67162` for the GFX driver.
*   **When:** The `hal_display_init` function is called from a test application.
*   **Then:** The function should return `true`.
*   **And:** The display should turn on. (Visual confirmation for user)

**Scenario: Clear Display to a Solid Color**
*   **Given:** The display has been successfully initialized using the `Arduino_GFX` driver.
*   **When:** The `hal_display_clear` function is called with the color `0xF800` (pure red).
*   **Then:** The implementation must call the underlying `g_gfx->fillScreen()` method.
*   **And:** The entire display should turn solid red. (Visual confirmation for user)

**Scenario: Draw a Single Pixel**
*   **Given:** The display has been successfully initialized and cleared.
*   **When:** `hal_display_draw_pixel(50, 50, 0x07E0)` is called.
*   **Then:** The implementation must call the underlying `g_gfx->drawPixel()` method.
*   **And:** A single green pixel should be visible at coordinates (50, 50). (Visual confirmation for user)

## 4. Build Configuration

The `platformio.ini` file must be updated for the `[env:tdisplay_s3_plus]` environment to correctly use the `Arduino_GFX` library.

**Scenario: Update platformio.ini for GFX Library**
*   **Given:** The `tdisplay_s3_plus` HAL is now based on `Arduino_GFX`.
*   **When:** The agent updates the build configuration.
*   **Then:** The `lib_extra_dirs` for `[env:tdisplay_s3_plus]` must be updated to include the path to the `GFX_Library_for_Arduino` and its dependencies.
*   **And:** The `build_flags` must be updated to remove `-Ihw-examples/LilyGo-AMOLED-Series/src` and add includes for the GFX library similar to the `[env:esp32s3]` environment.

## Implementation Notes

### [2026-02-11] TE (Tearing Effect) Sync
**Problem:** Visible tearing/shimmer during full-screen blits.
**Root Cause:** DMA transfers starting mid-frame conflict with the panel's refresh scan.
**Solution:** GPIO 9 is the TE output pin. `waitForTeSignal()` polls GPIO 9 for a rising edge (vertical blanking interval) before each `hal_display_fast_blit()` and `hal_display_fast_blit_transparent()`. This synchronizes DMA writes to the blanking period.
**Result:** Tearing reduced from "noticeable shimmer" to "barely perceptible." Residual artifacts are hardware limits (AMOLED pixel response time + DMA transfer duration).

### [2026-02-11] Brightness 255 Eliminates PWM Flicker
**Problem:** Intermediate brightness levels (1–254) cause visible PWM flicker artifacts.
**Root Cause:** The RM67162 panel driver modulates brightness via PWM. At sub-max duty cycles, the PWM frequency is perceptible.
**Solution:** Always set brightness to 255 (full-on). This effectively makes the backlight DC, eliminating flicker.

### [2026-02-08] Vendor Init Retry
**Problem:** Occasional `hal_display_init` failure on cold boot.
**Solution:** The RM67162 driver initialization includes a retry loop (up to 3 attempts) with a 100ms delay between attempts. This handles the panel's variable cold-start timing.

### [2026-02-07] Y-Offset Register Fix
**Problem:** Display content offset vertically on the T-Display S3 AMOLED Plus panel.
**Root Cause:** The RM67162 panel on this board has a non-zero vertical RAM offset that differs from the Waveshare ESP32-S3 AMOLED.
**Solution:** Set the correct Y-offset register during initialization to align the framebuffer with the physical panel origin.

### [2026-02-07] TE Sync — Detailed Timing Analysis
**Timing Mismatch:** Animation at 30fps (33.3ms/frame) vs display refresh at ~60Hz (16.6ms/frame). Without synchronization, beat frequency creates visible tearing when DMA blits occur mid-refresh.

**waitForTeSignal() pattern:**
1. Wait for LOW (display actively scanning)
2. Wait for HIGH (vertical blanking begins — safe to update)
3. Includes 10ms timeout to prevent infinite hangs if TE signal fails

**Key Lessons:**
- TE pin configuration ≠ TE pin usage — hardware init enables the signal, software must actively read it
- Top-to-bottom shimmer indicates timing mismatch (tearing), not PWM dimming
- Minimal performance impact: typically <1ms wait per frame at 30fps
- Even with perfect TE sync, AMOLED pixel response time (~2-5ms) and SPI transfer duration (~2-3ms) create inherent artifacts — "very slight" residual flicker is the physical limit