# Release v0.60: Initial Stock Tracker

> Label: "Release v0.60"
> Category: "Application Layer"
> Prerequisite: features/RELEASE_v0.58_dynamic_visuals.md

This release focuses on tracking a single stock (^TNX) using the Yahoo Finance API and displaying it on a redesigned graph.

## Features Included

- `features/ui_mini_logo.md`: A persistent mini-logo in the corner of the screen.
- `features/data_layer_stock_tracker.md`: A new data layer for fetching and managing stock data.
- `features/ui_themeable_time_series_graph_v2.md`: An updated time series graph with a new look and feel.
- `features/demo_release_0.60.md`: The demo for this release.

## Demo Flow

The demo for this release will have the following flow:

1.  **Logo Screen:** The demo starts with the logo screen, which animates to the top-right corner, becoming the "mini logo".
2.  **WiFi Connection:** The WiFi connection status screen appears. The mini logo remains in the top-right corner.
3.  **Connection Status:**
    - If the WiFi ping fails, an error message is displayed, and the demo pauses.
    - If the WiFi ping succeeds, the demo proceeds to the stock tracker graph.
4.  **Stock Tracker Graph:**
    - The graph of the ^TNX stock is displayed.
    - The mini logo remains in the top-right corner.
    - The text "DEMO v0.60" is displayed in the top-left corner.
    - The graph is updated periodically with new data fetched from the Yahoo Finance API.

## Validation Criteria

- The demo must follow the specified flow.
- The mini logo must be drawn correctly and efficiently.
- The HTTP calls for data fetching must not block the UI thread.
- The graph must be displayed with the new styling and update correctly.
- There should be no memory leaks.
- All HIL tests for the included features must pass.
