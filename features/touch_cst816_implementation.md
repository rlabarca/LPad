# Feature: Touch CST816 Implementation

> Label: "HAL: CST816 Impl"
> Category: "Hardware Layer"
> Prerequisite: features/hal_spec_touch.md

## 1. Introduction
This feature specifies the implementation of the `hal_spec_touch` contract for the supported hardware platforms using the CST816 touch controller. This includes the `esp32s3` (1.8" AMOLED) and `tdisplay_s3_plus` (1.91" AMOLED) environments.

## 2. Hardware Configuration

### 2.1 Build Environments
The implementation must support the following PlatformIO environments:
- `esp32s3` (1.8" AMOLED)
- `tdisplay_s3_plus` (1.91" AMOLED)

### 2.2 Pin Definitions
The implementation must use the following pin configurations based on the build environment:

**Environment: `esp32s3` (1.8" AMOLED)**
- **SDA:** GPIO 15
- **SCL:** GPIO 14
- **INT:** GPIO 21
- **RST:** Unused (Managed by PMU or internal)
- **I2C Address:** 0x15

**Environment: `tdisplay_s3_plus` (1.91" AMOLED)**
- **SDA:** GPIO 3
- **SCL:** GPIO 2
- **INT:** GPIO 21
- **RST:** Unused
- **I2C Address:** 0x15

### 2.3 Dependencies
- The implementation should utilize the `SensorLib` library (or equivalent CST816 driver) available in the project dependencies (`lib_deps`).
- It must be implemented in `hal/touch_cst816.cpp` (or similar board-specific files if necessary, e.g., `hal/touch_esp32_cst816.cpp`) and adhere to the `hal/touch.h` interface.

## 3. Implementation Requirements

### 3.1 Initialization Logic
- The `hal_touch_init` function must configure the I2C bus with the correct pins for the active environment.
- It must initialize the CST816 driver.
- It should handle the case where the touch controller is in sleep mode (if applicable).

### 3.2 Coordinate Transformation
- The implementation must ensure that `hal_touch_read` returns coordinates that align with the *logical* display orientation defined in `hal_display`.
- If the display HAL rotates the screen, the touch coordinates must be rotated to match (e.g., if the user touches the top-left of the visual interface, the coordinates must be (0,0) regardless of the physical panel orientation).

## 4. Scenarios

### Scenario: Board-Specific Pin Configuration
- Given the build environment is `esp32s3`
- When `hal_touch_init` is executed
- Then I2C should be initialized on SDA=15, SCL=14

### Scenario: Board-Specific Pin Configuration (Plus)
- Given the build environment is `tdisplay_s3_plus`
- When `hal_touch_init` is executed
- Then I2C should be initialized on SDA=3, SCL=2

### Scenario: Coordinate Output
- Given the touch controller reports raw X=100, Y=50
- And the display rotation matches the touch panel orientation
- When `hal_touch_read` is called
- Then it should return X=100, Y=50
