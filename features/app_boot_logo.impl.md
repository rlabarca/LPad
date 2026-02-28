# Implementation Notes: Boot Logo App

## Builder Decisions

**[AUTONOMOUS]** Added `getAnimState()` public accessor to `BootLogoApp` for state machine observability in tests. The spec requires state transitions to be verified but the original class exposed no state getter. This is a minimal, non-intrusive addition with no behavioral impact. (Severity: WARN)

**[AUTONOMOUS]** BootLogoApp was added to the native_test build via `+<apps/boot_logo_app.cpp>` in platformio.ini, deviating from the tool_test_runner spec's exclusion of the `apps/` directory. The pattern mirrors `+<system/power_manager.cpp>` already in use for system/ directory. (Severity: WARN)

**[AUTONOMOUS]** The HOLDING→DONE transition requires two `update()` ticks: the first tick transitions `m_state` to DONE, the second tick executes the DONE handler (calls `UIRenderManager::setActiveApp` and sets `m_transitioned = true`). This two-tick design means the app is observable in DONE state before the UIRenderManager side-effect fires. (Severity: INFO)

**[DISCOVERY]** `StockTracker::notifyResume()` was defined only inside `#ifdef ARDUINO` in `stock_tracker.cpp`, leaving it undefined on native when `stock_ticker_app.cpp` was added to the native build. A `#ifndef ARDUINO` no-op stub was added to `stock_tracker.cpp`. The data_stock_tracker spec should be updated to clarify native build behavior of this method. (Severity: HIGH) Acknowledged. Native no-op stubs for platform-specific APIs (FreeRTOS, Arduino) are test infrastructure decisions, not behavioral spec concerns. The data_stock_tracker spec correctly describes target-platform behavior; no spec change needed.

**[AUTONOMOUS]** `renderStatusText()` rewritten to use dirty-rect canvas blitting per `arch_display_pipeline.md`. The fixed text region is pre-computed from "Connecting..." max-width on the first CONNECTING render (`m_textRegionComputed` flag guards lazy init). All ellipsis variants left-align from `m_textCursorX` (fixed x-origin) — dots only extend right. `eraseStatusText()` simplified to use the pre-computed `m_textRegionX/Y/W/H` with a direct `fillRect` (only called once before DONE transition, not during animation). Canvas is `nullptr` in native tests due to stub, so `renderStatusText()` early-returns cleanly after setting `m_textRegionComputed`. (Severity: WARN)

## Test Coverage

Tests live in `test/test_boot_logo_app/test_boot_logo_app.cpp`. Covers all 15 automated scenarios from the spec: null pointer guards, state machine WAIT→ANIMATE→CONNECTING→DONE, ellipsis cycling, connected SSID display, early WiFi completion path, cross-core error signaling, one-shot error render, ellipsis position stability, dirty-rect blitting (smoke), network error text, and pre-DONE text erase.

**[AUTONOMOUS]** `getStatusText()` public accessor added to `BootLogoApp` to allow state machine text logic to be verified in tests without requiring GFX mock spy infrastructure. The method builds the same string that `renderStatusText()` would display, using a `mutable char m_statusTextBuf[64]` member. (Severity: WARN)

**[AUTONOMOUS]** `eraseStatusText()` is called directly from `update()` at the CONNECTING→DONE transition boundary (rather than deferring to the next `render()` call). This is intentional: by the time `render()` runs after the state transition, the active state is DONE, so a render()-based approach would need additional flags. The direct call from update() mirrors the one-time nature of the pre-DONE text wipe. (Severity: WARN)

**[AUTONOMOUS]** `hal_network_stub_set_ssid()` test helper added to `hal/network_stub.cpp` alongside the existing `hal_network_stub_set_status()`. The stub's `hal_network_get_ssid()` now returns a settable pointer (`g_stub_ssid`) rather than a hard-coded string literal. (Severity: WARN)

## Infrastructure Added

- `test/mocks/time_series_graph_stub.cpp`: no-op stub implementing all public `TimeSeriesGraph` methods for native builds; added to native_test `build_src_filter`
- `test/mocks/Arduino.h`: extended with minimal `HardwareSerial Serial` stub (no-op print/println/printf)
- `src/apps/boot_logo_app.h`: `getAnimState()` accessor
- `src/apps/stock_ticker_app.h`: UNIT_TEST-gated test accessors
- `src/data/stock_tracker.h`: UNIT_TEST-gated `setFetchStatusForTest()`
- `src/data/stock_tracker.cpp`: `#ifndef ARDUINO` stub for `notifyResume()`
