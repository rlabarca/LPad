# Feature: Bond Tracker Application

> **Prerequisite:**
> - `features/data_yahoo_chart_parser.md`
> - `features/ui_time_series_graph.md`

This feature defines the main application logic for the 10-Year Treasury Bond Tracker. It orchestrates the data parsing and UI rendering components to display the chart on the screen.

## Scenario: Application Startup and Initial Display

**Given** the device has booted up.
**And** the display HAL and `RelativeDisplay` have been initialized.

**When** the main application `setup()` function is executed.

**Then** an instance of the `YahooChartParser` should be created to parse `test_data/yahoo_chart_tnx_5m_1d.json`.
**And** an instance of the `TimeSeriesGraph` should be created with a vaporwave `GraphTheme`.
**And** the parsed data from the parser should be passed to the graph.
**And** the graph should be drawn to the display.

**And** the final output on the screen should show a line graph representing the bond price trend, rendered in the specified vaporwave theme.

## Implementation Notes

- The main application logic will reside in `src/main.cpp`.
- This feature ties together the `YahooChartParser` and `TimeSeriesGraph` modules.
- The `setup()` function will handle the one-time initialization. The `loop()` function may be empty for now or could be used to refresh the display.
- Error handling should be considered: if the data file cannot be parsed, the application should perhaps display an error message on the screen instead of crashing. This can be a separate scenario to be added later.
