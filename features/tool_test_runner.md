# Feature: Native Test Runner

> Label: "Tool: Test Runner"
> Category: "Developer Tools"
> Prerequisite: features/policy_build_pipeline.md
> Prerequisite: features/arch_hal_contract.md

[TODO]

## 1. Overview

The native test runner provides a local testing pipeline for LPad using PlatformIO's native platform with the Unity test framework. A dedicated `native_test` environment compiles test suites against HAL stubs and Arduino mocks, enabling host-native execution without embedded hardware. A shell wrapper script (`scripts/test_local.sh`) orchestrates test execution, captures JSON output, and persists results for CI and CDD integration.

---

## 2. Requirements

### 2.1 PlatformIO Test Environment

- Environment name: `native_test`. MUST be the default environment (`default_envs`).
- Platform: `native`. Test framework: `unity`.
- Build flags MUST include `-std=c++17`, `-DUNIT_TEST`, `-I.`, and `-Itest/mocks`.
- `test_build_src = yes` — application source is compiled alongside tests.
- Source filter MUST include all `src/` files except `main.cpp`, `ui_time_series_graph.cpp`, `animation_ticker.cpp`, `ui/ui_system_menu.cpp`, `apps/`, and `system/` (with `system/power_manager.cpp` re-included).
- Source filter MUST include HAL stubs: `display_stub.cpp`, `timer_stub.cpp`, `network_stub.cpp`, `touch_stub.cpp`, `power_stub.cpp`.
- Library dependencies: `ArduinoJson ^7.2.1`.

### 2.2 Test Ignore List

- `test_ignore` MUST list test suites that are known-broken or under development: `test_ui_time_series_graph`, `test_logo_screen`, `test_animation_ticker`, `test_vector_renderer`.
- Ignored tests MUST NOT cause the test run to fail.

### 2.3 Arduino Mocks

- `test/mocks/Arduino.h` MUST provide minimal Arduino API stubs sufficient for host-native compilation (types, `Serial`, `delay`, `millis`, etc.).
- `test/mocks/Arduino_GFX_Library.h` MUST provide a mock GFX library interface.
- Mocks are included via `-Itest/mocks` build flag.

### 2.4 Test Runner Script

- File: `scripts/test_local.sh`.
- MUST create `.pio/testing/` directory if absent.
- MUST execute `pio test -e native_test --json-output` and redirect output to `.pio/testing/last_summary.json`.
- MUST print completion message and display JSON results to stdout.
- MUST exit with non-zero status if `pio test` fails (`set -e`).

### 2.5 Test Suite Structure

- Each test suite lives in `test/<test_name>/test_<name>.cpp`.
- All tests use the Unity framework (`unity.h`).
- Test suites cover: data series, display HAL, display rotation, live indicator, logo screen, power manager, relative display, render manager, stock tracker, theme manager, mini logo, time series graph, vector renderer, and widget framework.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Native test environment compiles successfully

    Given the native_test environment is configured in platformio.ini
    When pio test -e native_test is executed
    Then all non-ignored test suites compile without errors

#### Scenario: HAL stubs link without hardware dependencies

    Given the native_test source filter includes all HAL stub files
    When test suites are compiled on the host
    Then no embedded SDK symbols are unresolved

#### Scenario: Ignored tests do not affect pass/fail status

    Given test_ignore lists test_ui_time_series_graph and others
    When pio test -e native_test is executed
    Then ignored suites are skipped
    And the exit code reflects only non-ignored test results

#### Scenario: Test runner script produces JSON summary

    Given scripts/test_local.sh is executed
    When all tests complete
    Then .pio/testing/last_summary.json exists
    And it contains valid JSON test results

#### Scenario: Mock Arduino.h provides required types

    Given a test file includes Arduino.h from test/mocks/
    When it uses uint8_t, uint16_t, int32_t, String, and Serial
    Then compilation succeeds on the native platform

### Manual Scenarios (Human Verification Required)

None.
