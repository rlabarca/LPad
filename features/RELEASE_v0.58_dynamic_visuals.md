# Release v0.58 - Dynamic Visuals

> Label: "Release v0.58 - Dynamic Visuals"
> Category: "Releases"
> Prerequisite: features/RELEASE_v0.55_connectivity_smoke_test.md
> Prerequisite: features/data_layer_time_series.md
> Prerequisite: features/demo_release_0.58.md

## 1. Release Capability Description
This release introduces dynamic data handling and real-time UI updates. It validates the integration between the `DataItemTimeSeries` model and the `TimeSeriesGraph` component, proving the system can ingest, store, and visualize live data feeds without performance degradation.

## 2. Integration Test Criteria
To validate this release, the system must successfully run the demo application defined in `features/demo_release_0.58.md` on the target hardware.

### Success Metrics
1.  **Data Continuity:** The graph correctly displays the logical continuation of the time series as new points are added.
2.  **Fixed-Window Buffer:** The `DataItemTimeSeries` maintains its fixed size, correctly dropping old points as new ones arrive.
3.  **Smooth Scrolling:** The graph visually "scrolls" to the left as new data points are injected, with no rendering artifacts.
4.  **Live Pulse:** The live indicator remains synchronized with the newest data point during updates.
5.  **Multi-Mode Stability:** Dynamic updates continue to function correctly across all 6 visual modes (Scientific/Compact, Gradient/Solid/Mixed).
