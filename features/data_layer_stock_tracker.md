# Data Layer: Stock Tracker

> Label: "Data Stock Tracker"
> Category: "Data Layer"
> Prerequisite: features/data_layer_time_series.md

This feature defines a data provider that fetches, parses, and manages time-series data for a specific stock symbol from the Yahoo Finance API.

## Scenarios

### Scenario: Initial Data Fetch

- **Given** a `StockTracker` instance configured for the "^TNX" symbol and a 1-minute refresh interval.
- **When** the `StockTracker` is started.
- **Then** it should perform an HTTP GET request to `https://query1.finance.yahoo.com/v8/finance/chart/%5ETNX?interval=5m&range=1d`.
- **And** the JSON response should be parsed successfully.
- **And** a `DataItemTimeSeries` instance should be created and populated with the last 30 minutes of data from the response.
- **And** the `DataItemTimeSeries` should be thread-safe.

### Scenario: Periodic Data Refresh

- **Given** a `StockTracker` that has successfully performed an initial data fetch.
- **When** the 1-minute refresh interval has elapsed.
- **Then** a new HTTP GET request is made to the Yahoo Finance API.
- **And** the new data points from the response are added to the `DataItemTimeSeries`, pushing out the oldest data points.
- **And** the `updated_at` timestamp of the `DataItemTimeSeries` is updated.

### Scenario: Threaded Operation

- **Given** the `StockTracker` is running and performing periodic updates.
- **When** an HTTP request is in progress.
- **Then** the main application loop and display rendering should not be blocked.
- **And** UI components can safely access the `DataItemTimeSeries` data without encountering partial updates (the data structure must be thread-safe).

## Implementation Details

- **`StockTracker` Class:**
  - This class will be responsible for managing the data fetching and parsing.
  - It should be configurable with a stock symbol and a refresh interval. For this release, these will be hardcoded to `^TNX` and 1 minute, respectively.
  - It will use the `Network` HAL for HTTP requests.
  - The HTTP request and JSON parsing should occur in a separate thread (or using an asynchronous callback mechanism) to avoid blocking the main thread.
  - It will own the `DataItemTimeSeries` instance.
  - It should provide a thread-safe method to get a pointer to the `DataItemTimeSeries` instance.

- **Data Parsing:**
  - A new JSON parsing implementation is required that can handle the Yahoo Finance API response format. The previous `YahooChartParser` is obsolete.
  - The parser should extract the timestamps and closing prices from the JSON response.

- **Thread Safety:**
  - The `DataItemTimeSeries` needs to be protected by a mutex or similar synchronization mechanism to ensure that the UI thread does not read the data while the network thread is writing to it. The `DataItemTimeSeries` should be extended to include this functionality.

## HAL Dependencies

- `hal_network` for making HTTP requests.
- `hal_timer` for scheduling periodic updates.

## Test Plan

- **Unit Test:**
  - Test the JSON parser with a sample valid JSON response and verify that the data is parsed correctly.
  - Test the JSON parser with invalid or incomplete JSON to ensure it handles errors gracefully.
  - Test the `StockTracker`'s ability to update the `DataItemTimeSeries` correctly.
- **Integration Test:**
  - An integration test should be created to verify that the `StockTracker` can successfully fetch data from the live Yahoo Finance API and update the data series. This may require a separate test environment with network access.
- **HIL Test:**
  - The main v0.60 demo will serve as the HIL test, visually confirming that the data is being fetched and displayed correctly.
