# Release v0.58: Dynamic Visuals

> Label: "Release v0.58: Dynamic Visuals"
> Category: "Release"
> Prerequisite: features/data_layer_time_series.md
> Prerequisite: features/demo_release_0.55.md

## Description
This release introduces dynamic, self-updating data to the application. It builds upon the visual foundation of v0.5/v0.55 but replaces the static data source with a live `DataItemTimeSeries` model. The demo simulates a real-time data feed by generating random updates, proving the system's ability to handle live data ingestion, model updates, and graph redrawing without UI stalls.

## Constraints
*   **No Regressions:** Must implement a new `V058DemoApp` class. The existing `V05DemoApp` and `V055DemoApp` classes MUST remain unchanged to preserve previous milestone behavior.
*   **Dispatcher Pattern:** Must hook into `main.cpp` via a new build flag (e.g., `-DDEMO_V058`).
*   **Structure:** Must maintain the "Connectivity -> Logo -> Graph Cycle" flow defined in v0.55.
*   **Visual Continuity:** The graph visual styles (gradients, layout) must match v0.5 exactly.

## Scenarios

### Scenario 1: Initial Data Seeding & Sizing
GIVEN the application starts in the Visual Phase
WHEN the `V058DemoApp` initializes
THEN it should load the initial dataset from `test_data/yahoo_chart_tnx_5m_1d.json`
AND it MUST create a `DataItemTimeSeries` with a `max_length` exactly equal to the number of points in that dataset
AND seed the instance with those points
AND the `TimeSeriesGraph` should be initialized with this data snapshot

### Scenario 2: Live Data Injection & Scrolling
GIVEN the graph is being displayed
WHEN 3 seconds of wall-clock time pass
THEN a new data point should be generated
AND its X-value MUST be the last X-value in the series plus the standard interval found in the seed data (logical continuation)
AND its Y-value should be random but within the current Y-axis bounds
AND this point should be added to the `DataItemTimeSeries` model
THEN the model MUST automatically drop the oldest data point to maintain its fixed size
AND the `V058DemoApp` MUST update the `TimeSeriesGraph` with the new data snapshot
AND the `TimeSeriesGraph` MUST redraw its data canvas
THEN the graph line should visually shift left (scrolling effect)

### Scenario 3: Mode Cycling with Live Data
GIVEN the demo is running
WHEN the application cycles through the 6 visual modes (Scientific/Compact x Gradient/Solid/Mixed)
THEN the live data updates should continue uninterrupted
AND the current data state (history + new points) should be preserved across mode switches

### Scenario 4: Connectivity Preservation
GIVEN the application starts
THEN it should first execute the Connectivity Phase (WiFi -> Ping) exactly as in v0.55
AND only proceed to the Dynamic Visual Phase after "PING OK" is displayed

### Scenario 5: Live Indicator Synchronization
GIVEN a new data point has been added and the graph has scrolled
WHEN the next frame is rendered
THEN the pulsing live indicator MUST automatically appear at the new rightmost data point
AND it MUST NOT leave "ghost" artifacts at the previous data point's position
AND the pulse animation MUST remain smooth and continuous during the data shift
