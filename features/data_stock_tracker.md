# Feature: Data Stock Tracker

> Label: "Data: Stock Tracker"
> Category: "Data Layer"
> Prerequisite: features/arch_concurrency.md
> Prerequisite: features/arch_hal_contract.md

[TODO]

## 1. Overview

The StockTracker fetches real-time financial data from the Yahoo Finance API on a FreeRTOS background task pinned to Core 0. It parses JSON responses into a DataItemTimeSeries ring buffer, manages fetch status states, and supports interruptible sleep for immediate resume after suspend/wake cycles. The tracker handles network outages gracefully by polling and waiting without stopping the task loop.

---

## 2. Requirements

### 2.1 Task Management

- `start()` creates a FreeRTOS task on Core 0 with 10KB stack. Returns false if already running or task creation fails.
- `stop()` sets the running flag to false and deletes the task.
- `notifyResume()` uses `xTaskNotifyGive()` to interrupt the task's sleep immediately for fast recovery after suspend.

### 2.2 Fetch Status States

- WAITING: initial state, no fetch attempted.
- NO_NETWORK: HAL reports DISCONNECTED or ERROR.
- HAS_DATA: successful fetch with renderable data points.
- NON_TRADING_HOURS: valid JSON but no timestamp array (market closed). Also set when only meta fallback data is available.
- FETCH_ERROR: HTTP failure, JSON parse error, gzip detected, or array mismatch.

### 2.3 Task Loop

- Polls `hal_network_get_status()` every 500ms when not connected.
- On CONNECTED: calls `fetchData()`.
- Sleep intervals: 10s after first-fetch failure, 30s after incremental failure, configurable refresh interval (default 60s) after success.
- Sleep uses `ulTaskNotifyTake()` (interruptible by `notifyResume()`).

### 2.4 Data Fetching

- API URL: `https://query1.finance.yahoo.com/v8/finance/chart/{symbol}?interval=1m&range=6h`.
- Response buffer: 32KB (PSRAM if available, else heap).
- First fetch: clears the data series and bulk-loads all points.
- Incremental fetch: appends only points with timestamps newer than the latest existing point.

### 2.5 JSON Parsing

- Validates JSON structure: `chart.result[0].timestamp[]` and `chart.result[0].indicators.quote[0].close[]`.
- Null close values are silently skipped.
- Array size mismatch between timestamps and close values returns false.
- Missing timestamp array with valid `meta.regularMarketPrice` triggers NON_TRADING_HOURS with a single meta-derived point.
- Gzip-compressed responses (first byte not `{`) are rejected.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Start creates task on Core 0

    Given a StockTracker instance
    When start is called
    Then a FreeRTOS task is created on Core 0
    And isRunning returns true

#### Scenario: Start rejects if already running

    Given the tracker is already running
    When start is called
    Then it returns false

#### Scenario: No network sets appropriate status

    Given the WiFi status is DISCONNECTED
    When the task loop polls network status
    Then getFetchStatus returns NO_NETWORK

#### Scenario: Successful fetch sets HAS_DATA

    Given WiFi is connected
    And the Yahoo Finance API returns valid data
    When fetchData completes
    Then getFetchStatus returns HAS_DATA
    And data points are added to the series

#### Scenario: First fetch clears and bulk-loads

    Given this is the first fetch (no prior data)
    When fetchData succeeds with 100 data points
    Then the data series is cleared
    And all 100 points are added

#### Scenario: Incremental fetch appends only new points

    Given the data series already has points up to timestamp T
    When fetchData returns points including timestamps before and after T
    Then only points with timestamp > T are appended

#### Scenario: Non-trading hours detected

    Given the API returns valid JSON but no timestamp array
    When parseYahooFinanceResponse is called
    Then getFetchStatus returns NON_TRADING_HOURS

#### Scenario: Gzip response is rejected

    Given the API returns a gzip-compressed response
    When parseYahooFinanceResponse checks the first byte
    Then it returns false with FETCH_ERROR status

#### Scenario: NotifyResume interrupts sleep

    Given the task is sleeping after a successful fetch
    When notifyResume is called
    Then the task wakes immediately and initiates the next fetch cycle

### Manual Scenarios (Human Verification Required)

#### Scenario: Live data fetching on device

    Given the device is connected to WiFi
    When the stock tracker is running
    Then new data points appear on the chart every 60 seconds
    And the fetch status reflects market hours correctly
