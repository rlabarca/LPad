# Release v0.60: Initial Stock Tracker

> Label: "Release v0.60"
> Category: "RELEASES"
> Prerequisite: features/RELEASE_v0.58_dynamic_visuals.md
> Prerequisite: features/demo_release_0.60.md

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
    - The graph is initialized with the past 6 hours of trading data and updated every minute with new data fetched from the Yahoo Finance API.
    - **Note:** "6 hours of trading data" refers to the last 6 hours when the market was open, not wall-clock time. During non-trading hours, this data will have timestamps from the previous trading session (e.g., 20+ real-world hours ago), and the X-axis will correctly display "Hours Prior" relative to those timestamps.

## Validation Criteria

- The demo must follow the specified flow.
- The mini logo must be drawn correctly and efficiently.
- The HTTP calls for data fetching must not block the UI thread.
- The graph must be displayed with the new styling and update correctly.
- There should be no memory leaks.
- All HIL tests for the included features must pass.
