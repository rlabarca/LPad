# Feature: Stock Ticker App

> Label: "App: Stock Ticker"
> Category: "Applications"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/arch_display_pipeline.md

[TODO]

## 1. Overview

The StockTickerApp is the main AppComponent (Z=1) that displays a live financial data chart. It owns a StockTracker (Yahoo Finance API fetcher) and a TimeSeriesGraph (three-layer PSRAM canvas renderer). The app renders status screens (waiting, no network, error, non-trading hours) when data is unavailable, and a live updating chart with pulsing indicator when data is present. Currently configured to track the 10-year US Treasury yield (^TNX).

---

## 2. Requirements

### 2.1 Lifecycle

- `begin(display)` creates the TimeSeriesGraph and StockTracker with symbol "^TNX", 60s refresh, 30min history. Returns false if display or GFX is null or graph init fails.
- `onRun()` starts the StockTracker background task.
- `onPause()` stops the StockTracker task (must happen before WiFi teardown to prevent LWIP corruption).
- `onUnpause()` resets all render flags and restarts the tracker, forcing a full redraw (AMOLED GRAM undefined after sleep).
- `onClose()` stops and deletes both tracker and graph.

### 2.2 Status Screens

- Status screens are shown based on FetchStatus, with priority:
  1. NON_TRADING_HOURS: always shown (even with existing data).
  2. FETCH_ERROR: shown only when no existing data.
  3. WAITING: shown only when no existing data.
  4. NO_NETWORK: shown only when no existing data.
- Status text is centered using theme normal font and text_main color.
- Status screens redraw only when the fetch status changes (tracked via `m_lastRenderedFetchStatus`).

### 2.3 Data Rendering

- When FetchStatus is HAS_DATA and data exists, the graph renders the time series.
- Data updates are detected by comparing `graphData.x_values.back()` to `m_lastDataTimestamp`.
- On new data: `setData()` -> `drawBackground()` -> `drawData()` -> `render()`.
- The live indicator animation runs via `m_graph->update(dt)` every frame after the first render.

### 2.4 Component Properties

- `isOpaque()` returns `true`. `isFullscreen()` returns `true`.
- `handleInput()` always returns `false` (all events pass to system components).

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Begin creates tracker and graph

    Given a valid RelativeDisplay
    When begin is called
    Then a StockTracker is created with symbol "^TNX"
    And a TimeSeriesGraph is created and initialized

#### Scenario: Status screen shown when waiting with no data

    Given the StockTracker fetch status is WAITING
    And no data points exist
    When render is called
    Then "Retrieving Data" is displayed centered on screen

#### Scenario: Non-trading hours always shown

    Given the StockTracker fetch status is NON_TRADING_HOURS
    And previous data points exist
    When render is called
    Then "Non Trading Hours" is displayed (overrides existing data)

#### Scenario: Graph renders when data available

    Given the StockTracker fetch status is HAS_DATA
    And the data series has new points since last render
    When render is called
    Then the TimeSeriesGraph draws background, data, and composites to display

#### Scenario: Pause stops tracker before WiFi teardown

    Given the StockTickerApp is running
    When onPause is called
    Then the StockTracker task is stopped

#### Scenario: Unpause forces full redraw

    Given the StockTickerApp was paused
    When onUnpause is called
    Then all render flags are reset
    And the next render redraws everything from scratch

### Manual Scenarios (Human Verification Required)

#### Scenario: Live chart updates on device

    Given the device is connected to WiFi and market is open
    When the stock ticker app is active
    Then the chart line updates every 60 seconds with new data points
    And a pulsing dot indicates the latest data point
