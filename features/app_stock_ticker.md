# Feature: App Component - Stock Ticker

> Label: "App Component: Stock Ticker"
> Category: "Applications"
> Prerequisite: features/core_ui_render_manager.md
> Prerequisite: features/data_layer_stock_tracker.md
> Prerequisite: features/ui_themeable_time_series_graph_v2.md

## 1. Overview
Encapsulate the stock tracking and graphing functionality (from `RELEASE_v0.60`) into a cohesive `AppComponent`. This serves as the primary "App" for the v0.70 release.

## 2. Configuration
*   **Type:** AppComponent
*   **Z-Order:** 1
*   **Name:** "StockTicker"

## 3. Behavior

### 3.1 Lifecycle
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
