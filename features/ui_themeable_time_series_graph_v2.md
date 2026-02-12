# UI Themeable Time Series Graph v2

> Label: "UI Time Series Graph v2"
> Category: "UI Framework"
> Prerequisite: features/ui_themeable_time_series_graph.md

This feature updates the `TimeSeriesGraph` component with a new look and feel, as specified for the v0.60 release.

## Scenarios

### Scenario: Graph with New Styling

- **Given** a `TimeSeriesGraph` instance.
- **When** the graph is rendered with a data series.
- **Then** the following styling rules should be applied:
  - The graph background should be a solid color matching the logo screen background.
  - All lines and indicators should be solid colors from the theme (no gradients).
  - Axis labels should be drawn *inside* the graph area, with a consistent padding offset such that they NEVER overlap or touch the axis lines.
  - The component MUST autonomously manage the spacing between axis lines and labels, dynamically adjusting internal margins based on label dimensions.
  - The component MUST autonomously manage tick and label placement to prevent collisions between labels or with the axis lines.
  - Origin Clearance: Labels for both X and Y axes MUST NOT be rendered at the origin to avoid mutual overlap and axis line interference.
  - The Y-axis should have labels with a maximum of 3 significant digits, dynamically adjusted to the data range.
  - **Precise Tick Placement:** Tick marks MUST be placed at exact "clean" values where all non-significant digits are zero. For example, if displaying 3 significant digits, a tick at `4.19` must be located at the exact vertical position representing `4.19000...`, rather than an arbitrary data point that rounds to `4.19`.
  - **Unique Label Constraint:** The component MUST ensure that no two Y-axis tick labels are identical after formatting. If the data range is small enough that standard increments produce identical formatted strings, the component MUST increase the precision or adjust the tick spacing to ensure every label is unique.
  - **Spacing & Clearance:** The component MUST maintain clear vertical spacing between Y-axis labels. The bottom-most Y-axis label MUST NOT overlap the X-axis line or its title.
  - The X-axis labels should represent hours prior to the latest data point (e.g., "0", "1", "2").
  - The number and spacing of X-axis ticks and labels should be dynamically chosen by the component to fit the screen width without overlapping, ensuring they represent logical hour intervals.
  - The Y-axis should have a title "Value" rendered vertically.
  - The X-axis should have a title "Hours Prior" rendered horizontally.
  - The graph must maintain a "Live" indicator (pulsing circle) at the latest data point.

### Scenario: Optional Ticker Watermark

- **Given** a `TimeSeriesGraph` instance.
- **When** the `setWatermark(const char* text)` method is called with a valid string (e.g., "^TNX").
- **Then** the text MUST be displayed at the top-center of the screen.
  - **Layering:** It MUST be drawn *under* all other graph elements (background layer).
  - **Font:** `fonts.heading` (second largest font).
  - **Color:** `colors.text_secondary` (most subtle text color).
- **But** if `setWatermark` has NOT been called (or called with `nullptr`), NO watermark text should be rendered. This ensures backward compatibility with previous demos.

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
