# UI Themeable Time Series Graph v2

> Label: "UI Time Series Graph v2"
> Category: "UI Framework"
> Prerequisite: features/ui_base.md

This feature updates the `TimeSeriesGraph` component with a new look and feel, as specified for the v0.60 release.

## Scenarios

### Scenario: Graph with New Styling

- **Given** a `TimeSeriesGraph` instance.
- **When** the graph is rendered with a data series.
- **Then** the following styling rules should be applied:
  - The graph background should be a solid color matching the logo screen background.
  - All lines and indicators should be solid colors from the theme (no gradients).
  - Axis labels should be drawn *inside* the graph area.
  - The Y-axis should have labels with a maximum of 3 significant digits.
  - The X-axis labels should represent minutes prior to the latest data point (e.g., "0", "3", "6").
  - The number and spacing of X-axis ticks and labels should be dynamically chosen to fit the screen width without overlapping.
  - The Y-axis labels should be dynamically calculated based on the min/max values of the current data, and the graph's Y-values should align perfectly with these labels.
  - The Y-axis should have a title "Value".
  - The X-axis should have a title "Mins Prior".

### Scenario: Dynamic Axis Labels

- **Given** a `TimeSeriesGraph` with a data series.
- **When** the data series is updated with new values that change the min/max range.
- **Then** the Y-axis labels are recalculated and redrawn to reflect the new range.
- **And** the X-axis labels are updated to reflect the new timestamps.

## Implementation Details

The `TimeSeriesGraph` class will be modified to support the new styling and layout options.

- **Axis Rendering:**
  - The logic for rendering axes and labels needs to be updated to support drawing labels inside the graph area.
  - A new algorithm is needed to determine the optimal number and placement of X-axis ticks and labels based on screen dimensions and data density.
  - Y-axis label generation must be updated to respect the 3-significant-digit requirement and to dynamically adjust to the data's range.

- **Theming:**
  - The `GraphTheme` struct should be updated or extended to control the new styling options, such as disabling gradients and specifying label positions.

- **Data-to-Screen Mapping:**
  - The mapping of data points to screen coordinates must be precise, ensuring that the plotted line aligns exactly with the corresponding axis labels.

## HAL Dependencies

- `hal_display` for drawing operations.

## Test Plan

- **Unit Test:**
  - Test the new axis label generation logic with various data ranges and screen sizes.
  - Verify that the graph renders correctly with the new styling options.
- **HIL Test:**
  - The main v0.60 demo will visually validate the new graph look and feel, including the dynamic axis labels and correct data plotting.
