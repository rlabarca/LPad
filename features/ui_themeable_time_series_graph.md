# Feature: Themeable and Animated Time Series Graph

> **Prerequisite:** `features/ui_time_series_graph.md`

This feature extends the `TimeSeriesGraph` with advanced theming and animation capabilities. It introduces a layered drawing approach for better performance and defines new graphical primitives for gradients, thick lines, and circles, which must be added to the `display_relative_drawing` abstraction.

---

## Part 1: Extensions to Drawing Abstractions

To support the new graph features, the `RelativeDisplay` module and the core `GraphTheme` data structure must be extended.

### New Data Structures

#### `LinearGradient`
A structure to define a linear color gradient.
- `angle_deg`: `float`, the angle of the gradient in degrees (e.g., 0 for left-to-right, 90 for top-to-bottom).
- `color_stops`: An array or vector of up to 3 `uint16_t` color values.

#### `RadialGradient`
A structure to define a radial color gradient.
- `center_x`, `center_y`: `float`, relative coordinates for the gradient's center.
- `radius`: `float`, the radius of the gradient in relative units.
- `color_stops`: An array or vector of 2 `uint16_t` color values (inner and outer).

#### `GraphTheme` (Extended)
The existing `GraphTheme` structure is updated to include:
- `backgroundGradient`: `LinearGradient`, for the graph background.
- `lineGradient`: `LinearGradient`, for the data line.
- `lineThickness`: `float`, thickness of the data line in relative percentage units.
- `axisThickness`: `float`, thickness of the axes in relative percentage units.
- `tickColor`: `uint16_t`, color of the axis tick marks.
- `tickLength`: `float`, length of tick marks in relative percentage units.
- `liveIndicatorGradient`: `RadialGradient`, for the pulsing "live" data point indicator.
- `liveIndicatorPulseSpeed`: `float`, cycles per second for the pulse animation.

### New `RelativeDisplay` Functions

The following functions must be added to `src/relative_display.h` and implemented in `src/relative_display.cpp`. They will likely require new underlying HAL primitives for efficient implementation.

- `display_relative_fill_rect_gradient(float x, float y, float w, float h, const LinearGradient& gradient)`
- `display_relative_draw_line_thick(float x1, float y1, float x2, float y2, float thickness, uint16_t color)`
- `display_relative_draw_line_thick_gradient(float x1, float y1, float x2, float y2, float thickness, const LinearGradient& gradient)`
- `display_relative_fill_circle_gradient(float center_x, float center_y, float radius, const RadialGradient& gradient)`

---

## Part 2: TimeSeriesGraph Component Enhancements

### API Changes

The `TimeSeriesGraph` class is modified:
- `draw()` is split into `drawBackground()` and `drawData()`. This allows the data to be redrawn frequently without the expense of redrawing the background.
- A new method `update(float deltaTime)` is added to handle animations like the pulsing indicator.

### Scenario: Rendering a Gradient Background

**Given** a `TimeSeriesGraph` is initialized with an extended `GraphTheme`.
**And** the theme specifies a 3-color `backgroundGradient` at a 45-degree angle.
**When** the `drawBackground()` method is called.
**Then** the graph's background area should be filled with a smooth linear gradient corresponding to the theme, drawn by `display_relative_fill_rect_gradient`.
**And** the X and Y axes should be drawn using `display_relative_draw_line_thick` with the theme's `axisThickness` and `axisColor`.

### Scenario: Drawing a Thick, Gradient Data Line

**Given** a `TimeSeriesGraph` has its background drawn.
**And** data has been provided via `setData()`.
**And** the theme specifies a `lineThickness` of 0.5 and a horizontal `lineGradient`.
**When** the `drawData()` method is called.
**Then** the data line should be drawn with the specified thickness and horizontal gradient, using one or more calls to `display_relative_draw_line_thick_gradient`.

### Scenario: Displaying Axis Tick Marks

**Given** a `TimeSeriesGraph` has its background drawn.
**And** a method `setYTicks(float increment)` has been called with a value of 10.
**When** `drawBackground()` is called.
**Then** horizontal tick marks should be drawn on the Y-axis at every 10 units of data value.
**And** the ticks should be drawn using `display_relative_draw_line_thick` with the theme's `tickColor` and `tickLength`.

### Scenario: Animating the "Live" Data Indicator

**Given** a `TimeSeriesGraph` is drawn with data.
**And** the theme includes a `liveIndicatorGradient` and a `liveIndicatorPulseSpeed` of 1.0.
**When** the `update(float deltaTime)` method is called repeatedly.
**Then** a filled circle should be rendered at the position of the last data point.
**And** the circle's radius should pulse (e.g., using a sine wave) once per second.
**And** the circle should be filled with the specified radial gradient using `display_relative_fill_circle_gradient`.
**And** calling `drawData()` should not erase the indicator.

### Scenario: Independent Refresh

**Given** a `TimeSeriesGraph` is fully drawn.
**When** new data is set via `setData()`.
**And** only `drawData()` is called.
**Then** the background gradient and axes should remain unchanged.
**And** only the data line and live indicator should be updated to reflect the new data.
