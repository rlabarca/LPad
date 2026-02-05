# Feature: UI Time Series Graph

> Label: "Time Series Graph"
> Category: "UI Framework"

> **Prerequisite:** `features/display_relative_drawing.md`

This feature describes a reusable UI component for rendering a time-series line graph. It uses the `RelativeDisplay` abstraction to allow for resolution-independent drawing and layout. The graph's appearance is defined by a theme configuration.

## Data Structures

### `GraphTheme`

A struct or class to define the visual style of the graph.

- `backgroundColor`: Color of the graph area.
- `lineColors`: A `std::vector` or array of colors for the data line.
    - If 1 color: Solid line.
    - If > 1 color: Linear gradient applied horizontally along the graph width (e.g., Left=Color[0], Right=Color[N]).
- `axisColor`: Color of the X and Y axis lines.
- `labelFont`: Font for axis labels.
- `labelColor`: Color for axis labels.
- `showTicks`: Boolean to enable/disable tick marks.
- `tickColor`: Color of the tick marks.
- `tickLen`: Length of the tick marks.

### `GraphData`

A struct or class to hold the data to be plotted.

- `x_values`: A collection of x-axis values (e.g., timestamps).
- `y_values`: A collection of y-axis values (e.g., prices).

## Scenario: Drawing a Graph with Gradient Line

**Given** a `GraphTheme` is defined with `lineColors` = {Cyan, Pink}.
**When** the `draw()` method is called with data points.
**Then** the line connecting the data points should change color smoothly from Cyan (at the far left X coordinate) to Pink (at the far right X coordinate).

## Scenario: Drawing Ticks

**Given** a `GraphTheme` has `showTicks` = true.
**When** the `draw()` method is called.
**Then** small tick marks should be drawn along the Y-axis (and optionally X-axis) corresponding to the label positions.
**And** they should use the `tickColor`.

## Scenario: Dynamic Scaling (unchanged)

**Given** a `TimeSeriesGraph` has been drawn with an initial dataset.
**When** a new `GraphData` object is provided via `setData()` where the range of Y values is significantly different.
**And** the `draw()` method is called.
**Then** the Y-axis labels should update to reflect the new min and max values.
**And** the plotted line should be re-scaled according to the new data range.

## Implementation Notes

- **Gradient Line Algorithm:**
    - The gradient is applied based on the *X-coordinate* of the segment being drawn, relative to the graph's width.
    - `t = (pixel_x - graph_x) / graph_width`.
    - Interpolate between the colors in `lineColors` based on `t`.
- **Canvas usage:** The component must rely exclusively on the `RelativeDisplay` or the active HAL target.
- **Vaporwave Theme:** This remains a key default style to support (Cyan->Pink gradient lines).