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
*   **Pause:**
    *   Suspends graph rendering updates to save cycles.
    *   *Optional:* Can pause the background data fetch task if power saving is required, but typically data continues to accumulate.
*   **Unpause:**
    *   Triggers a full graph redraw (`force_redraw = true`) to ensure clean state after being obscured.
    *   Resumes normal render loop.
*   **Close:**
    *   Stops the `StockTracker` task.
    *   Frees the `TimeSeriesGraph` resources (PSRAM buffers).

### 3.2 Interaction
*   **Input:**
    *   Responds to Touch Gestures (e.g., Tap to cycle modes, if implemented).
    *   Does NOT consume Edge Drags (allows them to bubble up to System Menu).

## 4. Scenarios

### Scenario: Background Updates
    Given the StockTicker is running
    And the System Menu is Active (StockTicker is Paused)
    When new stock data arrives from the network
    Then the StockTracker data model is updated
    But the Graph UI is NOT redrawn (saving cycles)

### Scenario: Resume Rendering
    Given the StockTicker was Paused
    When the System Menu closes (StockTicker Unpaused)
    Then the Graph triggers a full repaint
    And the Graph includes the new data points received while paused

## Implementation Notes

### [2026-02-12] GraphTheme Font-Passing Pattern
**Critical:** The `StockTickerApp` must NOT `#include "theme_manager.h"` directly. The theme manager pulls in 5 custom GFXfont data arrays (~100KB+), which causes memory pressure and `TG1WDT_SYS_RST` watchdog crashes during pixel-intensive graph rendering. Instead, fonts are passed through the `GraphTheme` struct fields (which use built-in bitmap fonts on PSRAM canvases — see `features/ui_themeable_time_series_graph.md`).

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
