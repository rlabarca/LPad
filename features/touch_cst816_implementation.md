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
