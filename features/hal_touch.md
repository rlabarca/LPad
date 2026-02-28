# Feature: HAL Touch

> Label: "HAL: Touch"
> Category: "HAL"
> Prerequisite: features/arch_hal_contract.md

[TODO]

## 1. Overview

The Touch HAL provides a hardware-agnostic C interface for capacitive touch input on LPad devices. It abstracts controller differences (CST816T on LilyGo T-Display S3 Plus, FT3168 on Waveshare) behind a unified API for initialization, non-blocking touch point reading, sleep/wake control, and gesture engine configuration. Both implementations use direct I2C register access (SensorLib was abandoned due to reliability issues) and include retry logic for I2C bus contention.

---

## 2. Requirements

### 2.1 Lifecycle

- `hal_touch_init()` MUST return `true` on successful initialization and `false` on hardware failure.
- `hal_touch_init()` MUST be idempotent: calling it after successful init returns `true` immediately.
- CST816T implementation MUST disable auto-sleep immediately after initialization (register 0xFE = 0x01) because the chip re-enters sleep within ~5 seconds otherwise, and there is no RST pin to wake it reliably.

### 2.2 Touch Reading

- `hal_touch_read(point)` MUST be non-blocking. It returns `true` on success and `false` only on null pointer or uninitialized state.
- When no finger is touching, the point MUST be returned with `is_pressed=false`, `x=0`, `y=0`.
- Transient I2C errors MUST NOT cause `hal_touch_read` to return `false`; instead, the function returns `true` with `is_pressed=false`.
- Touch coordinates MUST be clamped to display bounds before returning.
- CST816T: Coordinates >= 600 MUST be treated as garbage and converted to no-touch.
- CST816T: 0xFF sentinel byte (buf[2]) MUST be filtered as post-auto-sleep-disable instability.

### 2.3 Home Button

- CST816T MUST detect the virtual home button (coordinate 600,120 or 120,600) and report it as `is_home_button=true` with `is_pressed=false`, `x=0`, `y=0`.
- FT3168 MUST always report `is_home_button=false` (no home button hardware).

### 2.4 I2C Retry

- Both implementations MUST retry I2C reads (2 attempts) to suppress transient NACK errors caused by bus contention with the power HAL.

### 2.5 Sleep/Wake

- `hal_touch_sleep()` MUST mark the controller as uninitialized. It MUST NOT send deep sleep commands on CST816T (no RST pin means no reliable wake mechanism).
- `hal_touch_wake()` on CST816T MUST use INT pin toggling with escalating delays (up to 5 retries, 50-250ms pulse, 100-300ms settle) and `Wire.end()`/`Wire.begin()` between each attempt to clear stale I2C state.

### 2.6 Gesture Engine Configuration

- `hal_touch_configure_gesture_engine(engine)` MUST set hardware-appropriate edge zone thresholds on the gesture engine for the target board's resolution and orientation.
- T-Display S3 Plus (536x240 landscape): left=40, right=430, top=36, bottom=204 (~15% from each edge, ~70% center area).

### 2.7 Stub Requirements

- The stub MUST compile on host-native without embedded SDK dependencies.
- `hal_touch_init()` in the stub MUST return `true`.
- `hal_touch_read()` in the stub MUST return `true` with `is_pressed=false`, `is_home_button=false`, `x=0`, `y=0`.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Touch init succeeds on first call

    Given the touch controller hardware is present
    When hal_touch_init is called
    Then it returns true

#### Scenario: Double init is idempotent

    Given hal_touch_init has already succeeded
    When hal_touch_init is called again
    Then it returns true without reinitializing

#### Scenario: Read with no touch returns unpressed state

    Given the touch controller is initialized
    And no finger is touching the screen
    When hal_touch_read is called
    Then it returns true
    And the point has is_pressed=false
    And coordinates are x=0, y=0

#### Scenario: Read with null pointer returns false

    Given the touch controller is initialized
    When hal_touch_read is called with a null pointer
    Then it returns false

#### Scenario: I2C transient error does not fail the read

    Given the touch controller is initialized
    And an I2C NACK occurs on the first attempt
    When hal_touch_read is called
    Then it retries and returns true with is_pressed=false

#### Scenario: Touch coordinates are clamped to display bounds

    Given the touch controller reports coordinates beyond display dimensions
    When hal_touch_read is called
    Then the returned coordinates are clamped to the valid range

#### Scenario: Home button detected on CST816T

    Given the CST816T touch controller is active
    And the controller reports coordinate (600, 120)
    When hal_touch_read is called
    Then the point has is_home_button=true and is_pressed=false

#### Scenario: Stub returns default unpressed state

    Given the native_test stub is active
    When hal_touch_read is called
    Then it returns true with is_pressed=false, is_home_button=false, x=0, y=0

### Manual Scenarios (Human Verification Required)

#### Scenario: Touch input responds correctly on T-Display S3 Plus

    Given the firmware is running on LilyGo T-Display S3 AMOLED Plus
    When the user taps the center of the screen
    Then the touch point is registered at the correct screen location

#### Scenario: Touch input responds correctly on Waveshare

    Given the firmware is running on Waveshare ESP32-S3 1.8" AMOLED
    When the user taps the center of the screen
    Then the touch point is registered at the correct screen location
