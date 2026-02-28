# Feature: Data Time Series

> Label: "Data: Time Series"
> Category: "Data Layer"
> Prerequisite: features/arch_concurrency.md

[TODO]

## 1. Overview

DataItemTimeSeries is a thread-safe FIFO ring buffer that stores `(long x, double y)` data points with a fixed maximum capacity. It extends the abstract DataItem base class and provides FreeRTOS mutex-protected access for cross-core producer/consumer patterns. The ring buffer supports efficient append, min/max tracking, and atomic snapshot export for graph rendering.

---

## 2. Requirements

### 2.1 Ring Buffer

- Fixed capacity set at construction (e.g., 400 points for StockTracker).
- `addDataPoint(x, y)` appends a point. When full, the oldest point is evicted (FIFO).
- Points are stored in a circular buffer indexed by `head_idx`.

### 2.2 Thread Safety

- All public methods (`addDataPoint`, `getGraphData`, `clear`, `getLength`) MUST acquire the FreeRTOS mutex before accessing shared state.
- Mutex wait is `portMAX_DELAY` (indefinite). Operations return safe defaults if mutex is null.
- On native builds (UNIT_TEST), mutex operations are no-ops (single-threaded).

### 2.3 Min/Max Tracking

- `getMinVal()` and `getMaxVal()` track the current extremes of Y values.
- When an evicted point's Y equals the current min or max, a full O(n) recalculation is triggered.
- Otherwise, only the new point is compared against current extremes.

### 2.4 Graph Data Export

- `getGraphData()` returns a `GraphData` struct containing two vectors (`x_values`, `y_values`) copied in chronological order (oldest to newest).
- The snapshot is taken under the mutex lock to ensure consistency.
- Returns empty GraphData if lock fails or buffer is empty.

### 2.5 DataItem Base

- `DataItem(name)` stores a string identifier and a `lastUpdated` timestamp.
- `touch()` stamps `lastUpdated` with `hal_timer_get_micros()`. Called by `addDataPoint()` and `clear()`.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Add points up to capacity

    Given a DataItemTimeSeries with max_length 5
    When 5 data points are added
    Then getLength returns 5

#### Scenario: FIFO eviction when full

    Given a DataItemTimeSeries with max_length 3 containing points (1,10), (2,20), (3,30)
    When addDataPoint(4, 40) is called
    Then getLength is still 3
    And getGraphData returns points (2,20), (3,30), (4,40) in chronological order

#### Scenario: Min/max tracks current values

    Given points (1, 5.0), (2, 10.0), (3, 3.0) are added
    Then getMinVal returns 3.0
    And getMaxVal returns 10.0

#### Scenario: Min/max recalculates on eviction of extreme

    Given max_length 2 with points (1, 5.0), (2, 10.0)
    When addDataPoint(3, 7.0) evicts (1, 5.0)
    Then getMinVal returns 7.0
    And getMaxVal returns 10.0

#### Scenario: Graph data snapshot is chronological

    Given a ring buffer that has wrapped around
    When getGraphData is called
    Then x_values and y_values are ordered oldest to newest

#### Scenario: Clear resets all state

    Given a DataItemTimeSeries with data
    When clear is called
    Then getLength returns 0
    And getMinVal returns positive infinity
    And getMaxVal returns negative infinity

### Manual Scenarios (Human Verification Required)

None.
