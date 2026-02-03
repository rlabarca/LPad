# Feature: Bond Tracker Application

> **Prerequisite:**
> - `features/data_yahoo_chart_parser.md`
> - `features/ui_time_series_graph.md`
> - `features/display_canvas_drawing.md`

This feature defines the main application logic for the 10-Year Treasury Bond Tracker. It orchestrates data parsing and UI rendering, leveraging a canvas for accelerated, layered graphics.

## Scenario: Application Startup and Initial Display

**Given** the device has booted up.
**And** the display HAL and `RelativeDisplay` have been initialized.

**When** the main application `setup()` function is executed.

**Then** a full-screen canvas named `graph_canvas` should be created using `hal_display_canvas_create()`.
**And** the `graph_canvas` should be selected as the active drawing target using `hal_display_canvas_select()`.
**And** an instance of the `YahooChartParser` should be created to parse `test_data/yahoo_chart_tnx_5m_1d.json`.
**And** an instance of the `TimeSeriesGraph` should be created with a vaporwave `GraphTheme`.
**And** the parsed data from the parser should be passed to the graph.
**And** the graph should be drawn to the currently selected canvas (the `graph_canvas`).
**And** the main display should be re-selected as the drawing target via `hal_display_canvas_select(nullptr)`.
**And** the `graph_canvas` should be drawn to the main display at coordinates (0, 0) using `hal_display_canvas_draw()`.

**And** the final output on the screen should show a line graph representing the bond price trend, rendered in the specified vaporwave theme.
**And** the `graph_canvas` should be deleted using `hal_display_canvas_delete()` to free memory.

## Implementation Notes

- The main application logic will reside in `src/main.cpp`.
- This feature now uses the Canvas API to render the graph off-screen before blitting it to the display.
- The `TimeSeriesGraph` component must be modified to draw to the currently active HAL target (which could be the screen or a canvas). This will be specified in `features/ui_time_series_graph.md`.
