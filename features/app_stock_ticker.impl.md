# Implementation Notes: Stock Ticker App

## Builder Decisions

**[AUTONOMOUS]** Added `UNIT_TEST`-gated accessors to `StockTickerApp`: `getTrackerForTest()`, `getLastRenderedFetchStatusForTest()`, `getLastDataTimestampForTest()`, `isBackgroundDrawnForTest()`. These expose private render-state fields for behavioral verification. They compile only when `-DUNIT_TEST` is set (native_test environment). (Severity: WARN)

**[AUTONOMOUS]** Added `UNIT_TEST`-gated `setFetchStatusForTest()` to `StockTracker` to inject `FetchStatus` values during tests. Without this, the NON_TRADING_HOURS and HAS_DATA scenarios could not be deterministically exercised on native (no real network). (Severity: WARN)

**[AUTONOMOUS]** `StockTickerApp` was added to the native_test build via `+<apps/stock_ticker_app.cpp>` in `platformio.ini`. This requires:
1. `ui_time_series_graph.cpp` remaining excluded (PSRAM/ESP32-specific) with the `TimeSeriesGraph` interface satisfied by `test/mocks/time_series_graph_stub.cpp`
2. `Serial` stub in `test/mocks/Arduino.h`
(Severity: WARN)

## Test Coverage

Tests live in `test/test_stock_ticker_app/test_stock_ticker_app.cpp`. Covers all 6 automated scenarios: tracker/graph creation verification, status screen routing (WAITING, NON_TRADING_HOURS, HAS_DATA), lifecycle pause/unpause with tracker stop/start and render-flag resets.

Data injection for rendering tests: `tracker->getDataSeries()->addDataPoint(timestamp, price)` populates the ring buffer, and `setFetchStatusForTest(HAS_DATA)` bypasses the network path so the data rendering branch is exercised natively.

## Infrastructure Shared with app_boot_logo

See `features/app_boot_logo.impl.md` for the full list of infrastructure additions (Serial mock, TimeSeriesGraph stub, platformio.ini changes, notifyResume fix).
