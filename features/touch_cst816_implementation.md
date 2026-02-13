# Feature: Touch CST816 Implementation

> Label: "HAL: CST816 Impl"
> Category: "Hardware Layer"
> Prerequisite: features/hal_spec_touch.md

## 1. Introduction
This feature specifies the implementation of the `hal_spec_touch` contract for the T-Display S3 AMOLED Plus (1.91") using the CST816T touch controller via direct I2C register access.

> **Note:** The Waveshare ESP32-S3 1.8" AMOLED uses a **FT3168** touch controller (not CST816). Its implementation is in `hal/touch_ft3168.cpp`. This spec covers CST816 only.

## 2. Hardware Configuration

### 2.1 Build Environments
The CST816 implementation applies to:
- `tdisplay_s3_plus` (T-Display S3 AMOLED Plus 1.91")

### 2.2 Pin Definitions

**Environment: `tdisplay_s3_plus` (1.91" AMOLED)**
- **SDA:** GPIO 3
- **SCL:** GPIO 2
- **INT:** GPIO 21
- **RST:** Unused (INT pin toggle used as pseudo-reset)
- **I2C Address:** 0x15

### 2.3 Dependencies
- The implementation uses direct I2C register access (bypassing SensorLib for reliability — see `docs/IMPLEMENTATION_LOG.md`).
- It must be implemented in `hal/touch_cst816.cpp` and adhere to the `hal/touch.h` interface.

## 3. Implementation Requirements

### 3.1 Initialization Logic
- The `hal_touch_init` function must configure the I2C bus with the correct pins for the active environment.
- It must initialize the CST816 driver.
- It should handle the case where the touch controller is in sleep mode (if applicable).

### 3.2 Coordinate Transformation
- The implementation must ensure that `hal_touch_read` returns coordinates that align with the *logical* display orientation defined in `hal_display`.
- If the display HAL rotates the screen, the touch coordinates must be rotated to match (e.g., if the user touches the top-left of the visual interface, the coordinates must be (0,0) regardless of the physical panel orientation).

## 4. Scenarios

### Scenario: Board-Specific Pin Configuration (T-Display S3 Plus)
- Given the build environment is `tdisplay_s3_plus`
- When `hal_touch_init` is executed
- Then I2C should be initialized on SDA=3, SCL=2
- And the INT pin wake-up sequence should be performed before I2C init
- And auto-sleep should be disabled (register 0xFE = 0x01)

### Scenario: Coordinate Output
- Given the touch controller reports raw X=100, Y=50
- And the display rotation matches the touch panel orientation
- When `hal_touch_read` is called
- Then it should return X=100, Y=50

### Scenario: Virtual Home Button Detection
- Given the CST816T reports coordinates (600, 120) or (120, 600)
- When `hal_touch_read` is called
- Then `is_home_button` should be `true`
- And `is_pressed` should be `false`

## Implementation Notes

### [2026-02-10] SensorLib Bypass — Direct I2C Register Access
**Problem:** The SensorLib CST816 driver was unreliable — intermittent I2C failures and incorrect coordinate reads.
**Solution:** Bypass SensorLib entirely. `hal/touch_cst816.cpp` uses direct `Wire.beginTransmission()` / `Wire.requestFrom()` to read registers 0x02–0x06 (event, x_hi, x_lo, y_hi, y_lo). This gives full control over error handling and retry logic.

### [2026-02-10] I2C Retry for WiFi Interference
**Problem:** When WiFi is active, I2C reads to the CST816T occasionally return NACK or garbage data.
**Root Cause:** The ESP32-S3's shared radio/peripheral bus creates transient I2C bus contention during WiFi TX bursts.
**Solution:** `hal_touch_read()` retries the I2C transaction up to 3 times on failure, with a 1ms delay between attempts.

### [2026-02-09] INT Pin Pseudo-Reset (No RST Pin)
**Problem:** The T-Display S3 AMOLED Plus does not expose the CST816T RST pin.
**Solution:** Toggle the INT pin (GPIO 21) LOW for 20ms, then HIGH, before I2C initialization. This acts as a pseudo-reset, waking the controller from deep sleep. Without this, the first `hal_touch_init()` after cold boot fails silently.

### [2026-02-09] Auto-Sleep 5s Window & Register 0xFE
**Problem:** The CST816T enters auto-sleep after ~5 seconds of no touch, after which I2C reads return stale data or NACK.
**Solution:** During init, write `0x01` to register `0xFE` to disable auto-sleep. This keeps the controller awake and responsive indefinitely.

### [2026-02-09] 0xFF Buffer Detection
**Problem:** After the controller wakes from sleep or during I2C glitches, all 6 bytes read back as `0xFF`.
**Solution:** `hal_touch_read()` checks if all returned bytes are `0xFF` and treats this as "no touch" rather than passing garbage coordinates upstream.

### [2026-02-12] I2C Transient Errors During WiFi Activity (v0.70)
**Problem:** Sporadic `[E][Wire.cpp:515] i2cWriteReadNonStop returned Error -1` at ~14s and ~28s after boot. No visible touch or display impact.
**Root Cause:** CST816 I2C reads (`Wire.requestFrom()`) occasionally NACK during high WiFi interrupt activity (initial HTTP fetch to Yahoo Finance). ESP32-S3 WiFi and I2C share interrupt priority.
**Fix:** Added single retry loop in `cst_read_registers()`. If the first I2C transaction NACKs, immediately retry once (<1ms latency).
**Lesson:** On ESP32-S3, transient I2C NACKs during network activity are normal hardware behavior — add a retry rather than treating them as errors.

### [2026-02-11] Complete CST816T Register Map
- `0x00-0x06`: Touch status + coordinates (read 7 bytes in single transaction)
- `0x02`: Touch count (lower 4 bits)
- `0x03-0x04`: X coordinate (high nibble + low byte)
- `0x05-0x06`: Y coordinate (high nibble + low byte)
- `0xA7`: Chip ID (`0xB4`=CST816S, `0xB5`=CST816T, `0xB6`=CST816D, `0xB7`=CST820)
- `0xA9`: Firmware version
- `0xFE`: Auto-sleep disable (write `0x01`)
- `0xFA`: Interrupt mode (write `0x60` for touch+change)
- `0xE5`: Sleep control (write `0x03` to sleep)

### [2026-02-11] Home Button Filtering
The CST816T reports a virtual home button at (600, 120) or swapped (120, 600). These coordinates must be filtered in `hal_touch_read()` before coordinate processing — they are not valid touch points.

### [2026-02-11] Coordinate System — Direct Pass-Through
The CST816T reports coordinates directly in display space — NO rotation, NO scaling, NO axis swap needed. Touchable area from HIL: X: 2-536, Y: 46-239 (slightly smaller than full 536×240).

### [2026-02-11] Why SensorLib Failed (Two Issues)
1. SensorLib's `isPressed()` + `getPoint()` are two separate I2C transactions. The interrupt flag auto-clears after the first read, creating phantom RELEASE events that break the gesture state machine.
2. The library's initialization sequence didn't reliably wake the controller from sleep/gesture-only mode without a hardware RST pin.

### [2026-02-11] Critical Init Sequence (Order Matters)
1. **INT pin wake-up (pseudo-reset):** Drive GPIO 21 LOW for 50ms, then set to INPUT. MUST happen BEFORE `Wire.begin()`.
2. **I2C init:** `Wire.begin(TOUCH_SDA, TOUCH_SCL)` at 100kHz.
3. **Disable auto-sleep:** Write 0x01 to register 0xFE (re-enters sleep within ~5s without this).
4. **Set interrupt mode:** Write 0x60 to register 0xFA for touch+change interrupts.
