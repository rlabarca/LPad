# Feature: HAL Timer

> Label: "HAL: Timer"
> Category: "HAL"
> Prerequisite: features/arch_hal_contract.md

[TODO]

## 1. Overview

The Timer HAL provides a hardware-agnostic C interface for monotonic microsecond timing on LPad devices. It wraps `esp_timer_get_time()` on ESP32 targets and provides a weak-linked stub for host-native unit testing that can be overridden by test fixtures.

---

## 2. Requirements

### 2.1 Interface Contract

- `hal_timer_init()` MUST be idempotent and return `true` on success. On ESP32, it is a no-op because `esp_timer` auto-initializes.
- `hal_timer_get_micros()` MUST return a monotonic microsecond counter not affected by wall-clock adjustments.
- The counter wraps on `uint64_t` overflow (practically infinite for device lifetimes).

### 2.2 Stub Requirements

- Both stub functions MUST be declared `__attribute__((weak))` so test fixtures can override them.
- Default stub: `hal_timer_init()` returns `false`, `hal_timer_get_micros()` returns `0`.
- Test fixtures override these by defining the same functions without the weak attribute.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Timer init succeeds on ESP32

    Given the ESP32 platform is active
    When hal_timer_init is called
    Then it returns true

#### Scenario: Timer returns monotonically increasing values

    Given hal_timer_init has succeeded
    When hal_timer_get_micros is called twice with a delay between
    Then the second value is greater than the first

#### Scenario: Stub timer init returns false by default

    Given the native_test stub is active and no test override is provided
    When hal_timer_init is called
    Then it returns false

#### Scenario: Stub timer can be overridden by test fixture

    Given a test fixture defines hal_timer_get_micros without weak attribute
    When hal_timer_get_micros is called
    Then the test fixture's implementation is used

### Manual Scenarios (Human Verification Required)

None.
