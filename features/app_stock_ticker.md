# Feature: App Component - Stock Ticker

> Label: "App Component: Stock Ticker"
> Category: "Applications"
> Prerequisite: features/core_ui_render_manager.md
> Prerequisite: features/data_layer_stock_tracker.md
> Prerequisite: features/ui_themeable_time_series_graph.md

## 1. Overview
Encapsulate the stock tracking and graphing functionality (from `RELEASE_v0.60`) into a cohesive `AppComponent`. This serves as the primary "App" for the v0.70 release.

## 2. Configuration
*   **Type:** AppComponent
*   **Z-Order:** 1
*   **Name:** "StockTicker"

## 3. Behavior

### 3.1 Lifecycle
*   **Implementation:** `src/apps/stock_ticker_app.cpp`
*   **Init:**
    *   Initializes the `StockTracker` singleton (Data Layer).
    *   Initializes the `TimeSeriesGraph` (UI Layer).
*   **Run:**
    *   Starts the `StockTracker` background task.
    *   Enables graph rendering.
*   **Pause / Sleep:**
    *   Suspends graph rendering updates to save cycles.
    *   The background data fetch task is suspended. No data updates occur during sleep.
*   **Unpause / Wake:**
    *   Triggers a full graph redraw (`force_redraw = true`).
    *   Immediately initiates a network connection attempt, following established WiFi priorities.
    *   Upon successful connection, immediately triggers a fetch for the latest data.
    *   After the initial fetch completes, resumes the normal periodic data fetch cycle.
*   **Close:**
    *   Stops the `StockTracker` task.
    *   Frees the `TimeSeriesGraph` resources (PSRAM buffers).

### 3.2 Interaction
*   **Input:**
    *   Responds to Touch Gestures (e.g., Tap to cycle modes, if implemented).
    *   Does NOT consume Edge Drags (allows them to bubble up to System Menu).

## 4. Scenarios

### Scenario: Wake from Sleep
    Given the StockTicker was asleep (Paused)
    And the data fetch task was suspended
    When the device wakes up (StockTicker is Unpaused)
    Then the system immediately tries to connect to a network based on saved priorities
    And once connected, the StockTracker immediately fetches the latest data
    And only after the wake-up fetch is complete, the normal periodic update cycle is resumed

### Scenario: Resume Rendering
    Given the StockTicker was Paused
    When the System Menu closes (StockTicker Unpaused)
    Then the Graph triggers a full repaint
    And the Graph includes any new data points received just after wake

### Scenario: Display "Retrieving Data"
    Given the StockTicker is running
    And the data series is empty
    And the StockTracker is actively fetching data
    And there is no other error message to display
    When the UI renders the graph
    Then the graph area is cleared
    And an empty graph container is drawn
    And the title "Retrieving Data" is displayed in the center of the graph area
    And this state persists until data is successfully retrieved or an error occurs

### Scenario: No Network Available
    Given the StockTicker is running
    And the device has no network connection
    When the UI attempts to render the graph
    Then the graph area is cleared
    And an empty graph container is drawn
    And the title "No Network" is displayed in the center of the graph area
    And this state persists until a network connection becomes available

### Scenario: Data Fetching or Parsing Error
    Given the StockTicker is running
    And the StockTracker fails to retrieve or parse data (e.g. network error, invalid response)
    When the UI attempts to render the graph
    Then the graph area is cleared
    And an empty graph container is drawn
    And the title "Data Error" is displayed in the center of the graph area
    And the title font is derived from the currently active theme
    And this state persists, refreshing as a normal frame, until data becomes available again

### Scenario: Display Market Hours Status
    Given the StockTicker is running
    And the stock market is currently closed
    When the UI attempts to render the graph
    Then the graph area is cleared
    And an empty graph container is drawn
    And the title "Non Trading Hours" is displayed in the center of the graph area
    And the title font is derived from the currently active theme
    And this state persists, refreshing as a normal frame, until the market opens

## Implementation Notes

### [2026-02-12] GraphTheme Font-Passing Pattern
**Critical:** `ui_time_series_graph.cpp` must NOT `#include "theme_manager.h"` — it pulls in 5 custom GFXfont data arrays (~100KB+) causing memory pressure and `TG1WDT_SYS_RST` watchdog crashes during pixel-intensive graph rendering. Fonts are passed through the `GraphTheme` struct. `stock_ticker_app.cpp` CAN include `theme_manager.h` safely (it's the graph rendering compilation unit that matters).

### [2026-02-12] Watchdog Avoidance
Graph rendering (especially `drawBackground()` with gradient fills) is CPU-intensive. The component avoids watchdog resets by:
1. Using layered rendering (background drawn once, not every frame).
2. Never including large font data in the compilation unit.
3. Yielding to the system between heavy operations (via the `UIRenderManager` frame loop).

### [2026-02-12] WiFi Race Condition — Graph Appears 60s Late
**Problem:** Display and mini logo appeared immediately, but stock graph didn't render until ~60s after boot.
**Root Cause:** `hal_network_init()` is async (returns immediately with status `CONNECTING`). StockTracker's FreeRTOS task starts during `setup()` and immediately calls `fetchData()`, which fails because WiFi is still connecting. Task then sleeps for the full 60s refresh interval.
**Fix (v0.70):** Added wait-for-WiFi loop before first fetch (polls every 500ms, up to 15s timeout). On failed first fetch, retry after 5s instead of 60s.
**Fix (v0.71 — Supersedes):** Replaced the blocking while-loop with a single non-blocking polling loop per `arch_data_strategy.md §1`. The task now polls `hal_network_get_status()` every 500ms and only attempts `fetchData()` when connected. No timeout — the task polls indefinitely until connected or shut down via `m_is_running`. This eliminates all blocking waits, even in FreeRTOS tasks.
**Lesson:** Any background task depending on WiFi must wait for connection before its first network operation. `hal_network_init()` returning true only means the attempt started.

### [2026-02-06] DataItemTimeSeries FIFO Sizing
**Problem:** Initial `max_length` of 50 caused graph to compress horizontally as it accumulated points before FIFO kicked in.
**Fix:** Set `max_length` to match initial data count (15), maintaining constant sliding window. Oldest data evicted as new data arrives (true FIFO behavior).

### [2026-02-06] Fixed Y-Bounds Prevent Data Drift
**Problem:** Random data generated from current min/max created feedback loop — values trended toward zero over time.
**Fix:** Capture initial Y-bounds at startup, use fixed bounds for all random generation. Data oscillates within stable range indefinitely.

### [2026-02-06] Embedded Test Data vs Filesystem
Initial implementation used LittleFS filesystem to store test data, requiring separate filesystem upload. Switched to embedded C++ arrays (`test_data/test_data_tnx_5m.h`). Simpler deployment (single firmware upload), no filesystem dependencies.

### [2026-02-16] I2C Bus Contention During TLS Handshakes
**Problem:** Wire.cpp error flood — I2C timeouts for FT3168 touch and AXP2101 PMU during HTTPS requests. The StockTracker FreeRTOS task ran on Core 1 (same as Arduino loop), and TLS handshakes monopolized the CPU.
**Fix:** Pin stock tracker task to Core 0 via `xTaskCreatePinnedToCore()` (WiFi protocol stack already runs there). Increased stack from 8KB to 10KB for TLS headroom.

### [2026-02-16] HTTP Response Buffer Corruption
**Problem:** `http.getString()` builds a dynamic Arduino String character-by-character from the TLS stream; cross-core preemption caused bytes to be dropped, producing garbled JSON.
**Fix:** Replaced with stream-based `readBytes()` in `hal_network_http_get()`. Uses `http.getStreamPtr()` to read directly into the caller's pre-allocated PSRAM buffer. Handles both known Content-Length and chunked (Content-Length: -1) responses.

### [2026-02-16] FetchStatus Enum for Status Screens
Added `FetchStatus` enum (WAITING, HAS_DATA, NON_TRADING_HOURS, FETCH_ERROR) to StockTracker. Set in `fetchData()` and `parseYahooFinanceResponse()` based on failure mode. StockTickerApp::render() checks status and draws centered text ("Non Trading Hours" or "Data Error") using theme font when no chart data is available.

### [2026-02-16] StockTracker Task Must Be Woken After Suspend/Resume
**Problem:** The StockTracker FreeRTOS task uses `vTaskDelay(60000)` between fetches. If suspend happens mid-delay, the task resumes into the remaining delay on wake — user sees stale "Data Error" or last chart for up to 60s while WiFi is already reconnected.
**Fix:** Replaced `vTaskDelay()` with `ulTaskNotifyTake()` (same timeout behavior, but interruptible). Added `StockTracker::notifyResume()` which calls `xTaskNotifyGive()` to wake the task immediately. Called from `StockTickerApp::onUnpause()` during the resume path.

### [2026-02-16] "Data Error" Should Not Override Existing Chart Data
**Problem:** An interrupted fetch during suspend sets `m_fetch_status = FETCH_ERROR`. On resume, the render path showed "Data Error" even though the data series contained valid chart data from before suspend. Per spec §4, error persists "until data becomes available" — but data was already available.
**Fix:** `render()` now checks `dataSeries->getLength() > 0` before showing "Data Error." If existing data is present, the graph renders normally. "Data Error" only displays when the series is empty. NON_TRADING_HOURS always displays since it's informational.

### [2026-02-17] "Retrieving Data" Status Screen
**Problem:** When `FetchStatus::WAITING` and data series is empty (initial boot, or resume before first successful fetch), `render()` returned early with no visual output — user saw a blank graph area with no indication the device was working.
**Fix:** Added `WAITING` to the status-screen condition block alongside `NON_TRADING_HOURS` and `FETCH_ERROR`. Displays centered "Retrieving Data" text using theme font. Only shown when data series is empty (same guard as "Data Error"). Completes spec §4 Scenario: Display "Retrieving Data".

### [2026-02-17] "No Network" Status Screen
**Problem:** Spec §4 "No Network Available" scenario was unimplemented. When the device had no network, users saw "Retrieving Data" (WAITING) or "Data Error" (FETCH_ERROR) instead of the correct "No Network" message.
**Fix:** Added `NO_NETWORK` variant to `FetchStatus` enum. `taskLoop()` now sets `NO_NETWORK` when `hal_network_get_status()` returns DISCONNECTED or ERROR (but not CONNECTING — during CONNECTING, "Retrieving Data" is still correct). `render()` displays "No Network" centered text. Only shown when data series is empty (same guard as "Data Error" — existing chart data takes precedence).

### [2026-02-16] WiFi Auto-Retry After Reconnect Timeout
**Problem:** `hal_network_get_status()` transitions to ERROR after a 10s timeout and calls `WiFi.disconnect(true)`. No code retries — WiFi is permanently dead until reboot. After `esp_wifi_stop()` + light sleep, the first `WiFi.begin()` can take longer than usual.
**Fix:** Added auto-retry logic in `hal_network_get_status()`: when status is ERROR and stored credentials exist, retries up to 3 times with 5s backoff between attempts. Counter resets on successful connection or explicit `hal_network_reconnect()` call.
