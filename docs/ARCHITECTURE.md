# System Architecture

## 1. High-Level Overview
The system follows a strict **Layered Architecture** designed to decouple Application Logic from Hardware Implementation.

```mermaid
graph TD
    App[Application (src/)] --> HAL_Contract[HAL Contract (hal/display.h)]
    HAL_Contract --> HAL_Stub[Stub Implementation (hal/display_stub.cpp)]
    HAL_Contract --> HAL_ESP32[ESP32 Implementation (hal/display_esp32_*.cpp)]
    
    subgraph "Application Layer"
    App
    RelativeDisplay[Relative Display Abstraction]
    TimeSeriesGraph[UI Component]
    AnimationTicker[Animation Loop]
    end

    subgraph "HAL Layer"
    HAL_Contract
    HAL_Stub
    HAL_ESP32
    end
```

## 2. Core Architectural Rules (The Constitution)

### A. The HAL Barrier
1.  **Strict Separation:** Application code (`src/*.cpp`) MUST NOT include hardware-specific headers (e.g., `esp_lcd_panel_io.h`) or vendor libraries directly. It must ONLY include `hal/display.h`.
2.  **No `Arduino.h` in Logic:** Application logic should minimize reliance on `Arduino.h`. Where used (e.g., `setup()`, `loop()`, or basic types), it must not couple logic to specific board capabilities.
3.  **The Stub Pattern:** Every HAL contract MUST have a corresponding `_stub.cpp` implementation. This ensures the application can always compile and run (logic-only) on any platform, even without specific hardware support.

### B. Data Flow
1.  **Unidirectional:** Data flows from Source -> Parser -> Model -> View.
    *   `YahooChartParser` (Source) -> `BondTracker` (Model) -> `TimeSeriesGraph` (View) -> `Display` (HAL).
2.  **Immutability:** Parsed data should be treated as immutable once passed to the visualization layer.

### C. Display Architecture
1.  **Relative Coordinates:** The Application Layer operates exclusively in a **0.0 to 1.0** float coordinate space.
    *   `x=0.0` is Left, `x=1.0` is Right.
    *   `y=0.0` is Bottom, `y=1.0` is Top.
    *   The `RelativeDisplay` module is the *only* component authorized to translate these floats into integer pixel coordinates.
2.  **Layered Rendering (The "Canvas" Strategy):**
    *   To prevent flickering and support complex composition, we use an off-screen canvas strategy.
    *   **Background Layer:** Static elements (grid, axes) are drawn once to a persistent canvas.
    *   **Data Layer:** Dynamic elements (graphs) are drawn to a separate canvas.
    *   **Composition:** The HAL is responsible for blitting these canvases to the physical display using `hal_display_fast_blit`.
3.  **Partial Updates:** Full screen clears (`hal_display_clear`) are prohibited in the `loop()`. Updates must use dirty-rect logic or optimized blitting of small regions (e.g., the pulsing indicator).

### D. State Management
1.  **The Ticker:** The `AnimationTicker` is the single source of truth for time.
2.  **Frame Rate:** The system targets a stable 30fps.
3.  **Delta Time:** All animations (e.g., pulsing indicator) must be calculated based on `deltaTime` provided by the ticker, never on raw frame counts.

## 3. Implementation Patterns

### HAL Implementation
*   **DMA Mandate:** All display drivers MUST use DMA (Direct Memory Access) for pixel transfers where hardware supports it.
*   **Blocking vs. Non-Blocking:** Drawing commands are generally blocking until data is *sent* to the DMA buffer, but return before the transfer is complete on the wire. `hal_display_flush()` acts as the synchronization barrier.

### Feature Flagging
*   **`platformio.ini` as Source of Truth:** Hardware targeting is controlled entirely by build flags in `platformio.ini`. Code uses `#ifdef` guards based on these flags to select the active HAL implementation at compile time.
