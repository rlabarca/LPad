# Implementation Notes: Boot Logo App

## Builder Decisions

**[AUTONOMOUS]** Added `getAnimState()` public accessor to `BootLogoApp` for state machine observability in tests. The spec requires state transitions to be verified but the original class exposed no state getter. This is a minimal, non-intrusive addition with no behavioral impact. (Severity: WARN)

**[AUTONOMOUS]** BootLogoApp was added to the native_test build via `+<apps/boot_logo_app.cpp>` in platformio.ini, deviating from the tool_test_runner spec's exclusion of the `apps/` directory. The pattern mirrors `+<system/power_manager.cpp>` already in use for system/ directory. (Severity: WARN)

**[AUTONOMOUS]** The HOLDING→DONE transition requires two `update()` ticks: the first tick transitions `m_state` to DONE, the second tick executes the DONE handler (calls `UIRenderManager::setActiveApp` and sets `m_transitioned = true`). This two-tick design means the app is observable in DONE state before the UIRenderManager side-effect fires. (Severity: INFO)

**[DISCOVERY]** `StockTracker::notifyResume()` was defined only inside `#ifdef ARDUINO` in `stock_tracker.cpp`, leaving it undefined on native when `stock_ticker_app.cpp` was added to the native build. A `#ifndef ARDUINO` no-op stub was added to `stock_tracker.cpp`. The data_stock_tracker spec should be updated to clarify native build behavior of this method. (Severity: HIGH) Acknowledged. Native no-op stubs for platform-specific APIs (FreeRTOS, Arduino) are test infrastructure decisions, not behavioral spec concerns. The data_stock_tracker spec correctly describes target-platform behavior; no spec change needed.

## Test Coverage

Tests live in `test/test_boot_logo_app/test_boot_logo_app.cpp`. Covers all 7 automated scenarios from the spec: null pointer guards, state machine WAIT→ANIMATE→HOLDING→DONE, cross-core error signaling, and one-shot error render. The error-render-once test is a smoke test (renders don't crash; the guard flag `m_errorRendered` prevents redundant draws).

## Infrastructure Added

- `test/mocks/time_series_graph_stub.cpp`: no-op stub implementing all public `TimeSeriesGraph` methods for native builds; added to native_test `build_src_filter`
- `test/mocks/Arduino.h`: extended with minimal `HardwareSerial Serial` stub (no-op print/println/printf)
- `src/apps/boot_logo_app.h`: `getAnimState()` accessor
- `src/apps/stock_ticker_app.h`: UNIT_TEST-gated test accessors
- `src/data/stock_tracker.h`: UNIT_TEST-gated `setFetchStatusForTest()`
- `src/data/stock_tracker.cpp`: `#ifndef ARDUINO` stub for `notifyResume()`
