# Feature: Bond Tracker Application

> **Prerequisite:**
> - `features/data_yahoo_chart_parser.md`
> - `features/ui_themeable_time_series_graph.md`
> - `features/display_canvas_drawing.md`

This feature defines the main application logic for the 10-Year Treasury Bond Tracker. It orchestrates data parsing and UI rendering, leveraging a canvas for accelerated, layered graphics and including a pulsing live data indicator.

## Scenario: Application Startup and Initial Display

**Given** the device has booted up.
**And** the display HAL and `RelativeDisplay` have been initialized.

**When** the main application `setup()` function is executed.

**Then** a full-screen canvas named `graph_canvas` should be created using `hal_display_canvas_create()`.
**And** the `graph_canvas` should be selected as the active drawing target using `hal_display_canvas_select()`.
**And** an instance of the `YahooChartParser` should be created to parse `test_data/yahoo_chart_tnx_5m_1d.json`.
**And** an instance of the `TimeSeriesGraph` should be created with a vaporwave `GraphTheme` that includes a `liveIndicatorGradient`.
**And** the parsed data from the parser should be passed to the graph.
**And** the graph's `drawBackground()` and `drawData()` methods should be called to draw to the `graph_canvas`.
**And** the main display should be re-selected as the drawing target via `hal_display_canvas_select(nullptr)`.

## Scenario: Application Loop and Animation

**Given** the application has been set up as in the previous scenario.

**When** the main application `loop()` function is executed repeatedly.

**Then** a `deltaTime` value should be calculated based on the time since the last loop iteration.
**And** the `graph_canvas` should be selected as the active drawing target.
**And** the graph's `update(deltaTime)` method should be called to animate the live indicator.
**And** the main display should be re-selected as the drawing target.
**And** the `graph_canvas` should be drawn to the main display at coordinates (0, 0) using `hal_display_canvas_draw()`.

## Implementation Notes

- The main application logic will reside in `src/main.cpp`.
- This feature now uses the Canvas API to render the graph off-screen before blitting it to the display.
- The `loop()` function is now responsible for updating the graph's animation and re-drawing the canvas to the screen on each frame.
- The `setup()` function no longer draws the canvas to the screen or deletes it; this is now handled by the loop.
