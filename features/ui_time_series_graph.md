# Feature: UI Time Series Graph

> **Prerequisite:** `features/display_relative_drawing.md`

This feature describes a reusable UI component for rendering a time-series line graph. It uses the `RelativeDisplay` abstraction to allow for resolution-independent drawing and layout. The graph's appearance is defined by a theme configuration.

## Data Structures

### `GraphTheme`

A struct or class to define the visual style of the graph.

- `backgroundColor`: Color of the graph area.
- `lineColor`: Color of the data series line.
- `axisColor`: Color of the X and Y axis lines.
- `labelFont`: Font for axis labels.
- `labelColor`: Color for axis labels.

### `GraphData`

A struct or class to hold the data to be plotted.

- `x_values`: A collection of x-axis values (e.g., timestamps).
- `y_values`: A collection of y-axis values (e.g., prices).

## Scenario: Initializing and rendering an empty graph

**Given** a `RelativeDisplay` instance is available.
**And** a `GraphTheme` is defined with "vaporwave" colors (e.g., background: dark purple, line: cyan, labels: magenta).
**When** a `TimeSeriesGraph` is initialized with the `RelativeDisplay` instance and the `GraphTheme`.
**And** the graph's `draw()` method is called without any data.
**Then** the graph's drawing area should be filled with the theme's `backgroundColor`.
**And** the X and Y axes should be drawn using the theme's `axisColor`.

## Scenario: Drawing a graph with data

**Given** a `TimeSeriesGraph` is initialized as in the previous scenario.
**And** a `GraphData` object is created with a series of X and Y values.
**When** the `setData()` method is called with the `GraphData` object.
**And** the `draw()` method is called.
**Then** a continuous line should be drawn connecting the data points, using the theme's `lineColor`.
**And** the data points should be scaled to fit within the graph's drawing area.
**And** labels for the min and max values of the Y-axis should be displayed using the theme's font and color.

## Scenario: Dynamically scaling the axes

**Given** a `TimeSeriesGraph` has been drawn with an initial dataset.
**When** a new `GraphData` object is provided via `setData()` where the range of Y values is significantly different.
**And** the `draw()` method is called.
**Then** the Y-axis labels should update to reflect the new min and max values.
**And** the plotted line should be re-scaled according to the new data range.

## Implementation Notes

- The `TimeSeriesGraph` should be implemented in its own module (e.g., `src/ui_time_series_graph.h`, `src/ui_time_series_graph.cpp`).
- The component must rely exclusively on the `RelativeDisplay` for all drawing operations (`draw_line`, `fill_rect`, `print_text`, etc.). **This ensures that it draws to the currently active HAL target (which could be the main display or an off-screen canvas).**
- The "vaporwave" theme should be the default or an easily selectable configuration.
- The component should internally manage the mapping of data coordinates (e.g., timestamps, prices) to the relative screen coordinates provided by `RelativeDisplay`.
