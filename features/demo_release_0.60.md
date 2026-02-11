# Demo for Release v0.60

> Label: "Demo v0.60"
> Category: "Release Demos"
> Prerequisite: features/demo_release_0.58.md
> Prerequisite: features/ui_mini_logo.md
> Prerequisite: features/data_layer_stock_tracker.md
> Prerequisite: features/ui_themeable_time_series_graph_v2.md
> Prerequisite: features/ui_connectivity_status_screen.md

This feature defines the implementation of the demo for release v0.60.

## Scenarios

### Scenario: Run Demo

- **Given** the device is powered on.
- **When** the demo for release v0.60 is started.
- **Then** the demo flow specified in `features/RELEASE_v0.60_initial_stock_tracker.md` is executed.

## Implementation Details

A new `v060_demo_app` will be created to orchestrate the demo.

- **`v060_demo_app` Class:**
  - This class will manage the state of the demo, including the current stage (logo, wifi, graph).
  - It will instantiate and manage the `LogoScreen`, `ui_connectivity_status_screen`, `StockTracker`, and `TimeSeriesGraph` components.
  - The `update()` method will handle the state transitions and update the components.
  - The `render()` method will draw the current state to the display.

- **Main Entry Point:**
  - `main.cpp` will be updated to run the `v060_demo_app`.

## HIL Test

The HIL test for this feature is the successful execution of the demo on the hardware, as observed by the user. The following should be verified:
- The logo animation plays correctly and transitions to the mini logo in the top-right corner.
- The WiFi connection screen appears, and the connection status is displayed.
- If the connection is successful, the stock graph appears.
- The graph displays the ^TNX data with the correct styling.
- The graph updates periodically without flickering or blocking the UI.
- The "DEMO v0.60" text and the mini logo are displayed correctly on the graph screen.
