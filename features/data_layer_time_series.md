# Data Layer: Time Series Data Item

> Label: "Time Series Data Item"
> Category: "Data Layer"
> Prerequisite: features/data_layer_core.md

## Description
This feature implements `DataItemTimeSeries`, a concrete subclass of `DataItem` specialized for storing ordered chronological data. It acts as a specialized FIFO (First-In-First-Out) ring buffer that maintains a fixed history of data points while automatically calculating statistical metadata (min/max) required for efficient graph scaling.

## Constraints
*   **Inheritance:** Must publicly inherit from `DataItem`.
*   **Fixed Capacity:** The maximum number of data points (`max_length`) is defined at construction and cannot change (to avoid expensive reallocations).
*   **Performance:** Adding a data point must be $O(1)$ or very close to it. Recalculating min/max can be optimized but must ensure correctness.
*   **Data Integrity:** The order of data points must strictly follow insertion order (oldest -> newest).
*   **Thread Safety:** The data structure must be thread-safe to allow concurrent access from the data provider thread (writing) and the UI thread (reading). This should be implemented using a mutex.

## Scenarios

### Scenario 1: Fixed Capacity FIFO Behavior
GIVEN a `DataItemTimeSeries` initialized with a `max_length` of 5
AND I have already added 5 data points [10, 20, 30, 40, 50]
WHEN I add a new data point 60
THEN the internal collection should contain 5 items
AND the items should be [20, 30, 40, 50, 60] (the oldest item 10 is removed)
AND the `curr_length` should remain 5

### Scenario 2: Automatic Range Calculation
GIVEN a `DataItemTimeSeries` containing values [100, 50, 200]
WHEN I query `getMinVal()` and `getMaxVal()`
THEN `getMinVal()` should return 50
AND `getMaxVal()` should return 200
WHEN I add a new value 300
THEN `getMaxVal()` should update to 300

### Scenario 3: Dynamic Range Updates on Removal
GIVEN a `DataItemTimeSeries` with `max_length` 3
AND it contains [10, 20, 30]
WHEN I add the value 25
THEN the values become [20, 30, 25]
AND `getMinVal()` should be 20 (since 10 was removed)

### Scenario 4: Export to Graph
GIVEN a `DataItemTimeSeries` with data
WHEN I call a method to retrieve the graph data (e.g., `getGraphData()`)
THEN it should return a standard `GraphData` struct (as defined in `ui_time_series_graph.h`)
AND the x_values and y_values in the struct should match the internal data
