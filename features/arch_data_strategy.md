# Architectural Policy: Data Strategy

> Label: "Data Strategy"
> Category: "ARCHITECTURES"
> Prerequisite: None

## 1. Connectivity Architecture
*   **Async-First:** `hal_network_init()` initiates the connection process but returns immediately.
*   **State Polling:** The application monitors connection state via `hal_network_get_status()` in the main loop or dedicated tasks. 
*   **No Blocking Loops:** **Blocking `while` loops (e.g., waiting for WiFi) are strictly prohibited**, even in dedicated FreeRTOS tasks.
    *   **Task Pattern:** Background tasks MUST use a state machine or periodic polling with `vTaskDelay()` to avoid starvation and allow the task to remain responsive to system signals.
*   **Domain Abstraction:** HTTP operations use a dedicated `hal_http_client` contract to decouple logic from the specific networking library.

## 2. Data Abstraction Hierarchy
*   **DataItem:** Abstract base class defining common metadata (name, timestamp, value, min/max tracking).
*   **DataItemTimeSeries:** Specialized subclass implementing a **fixed-capacity FIFO ring buffer**.
    *   **Capacity:** Fixed at construction (e.g., 15 points for sliding window).
    *   **FIFO Eviction:** When buffer is full, oldest data point is automatically removed.
    *   **Automatic Statistics:** Min/max values updated on every `addDataPoint()` call.

## 3. Data Flow Patterns
*   **Live Data:** Random Injection / API Fetch -> `addDataPoint()` -> FIFO Update -> View Refresh.
*   **Immutability:** Parsed data should be treated as immutable once passed to the visualization layer.
