# System Architecture

## 1. High-Level Overview
The system follows a strict **Layered Architecture** designed to decouple Application Logic from Hardware Implementation.

```mermaid
graph TD
    App[Application (src/)] --> HAL_Core[HAL Core Contract]
    
    subgraph "HAL Layer (Domain Segmented)"
    HAL_Core --> Spec_Disp[Display Spec]
    HAL_Core --> Spec_Net[Network Spec]
    HAL_Core --> Spec_Time[Timer Spec]
    
    Spec_Disp --> Driver_Disp[Display Driver (ESP32-S3)]
    Spec_Net --> Driver_Net[Network Driver (WiFi)]
    Spec_Time --> Driver_Time[Timer Driver (ESP32)]
    end
    
    App --> Spec_Disp
    App --> Spec_Net
    App --> Spec_Time

    subgraph "Application Logic"
    RelativeDisplay[Relative Display Abstraction]
    TimeSeriesGraph[UI Component]
    AnimationTicker[Animation Loop]
    ConfigSystem[Config Bridge]
    end
```

## 2. Core Architectural Rules (The Constitution)

### A. The HAL Barrier
1.  **Strict Separation:** Application code (`src/*.cpp`) MUST NOT include hardware-specific headers (e.g., `esp_lcd_panel_io.h`) or vendor libraries directly. It must ONLY include domain-specific HAL headers (e.g., `hal/display.h`, `hal/network.h`).
2.  **No `Arduino.h` in Logic:** Application logic should minimize reliance on `Arduino.h`. Where used (e.g., `setup()`, `loop()`, or basic types), it must not couple logic to specific board capabilities.
3.  **The Stub Pattern:** Every HAL contract MUST have a corresponding `_stub.cpp` implementation. This ensures the application can always compile and run (logic-only) on any platform, even without specific hardware support.

### B. Configuration Management (The Secret Store)
1.  **Zero-Secret Repository:** Credentials (Wi-Fi SSID, Passwords, API Keys) MUST NEVER be committed to the repository.
2.  **The `config.json` Bridge:** A `config.json` file in the project root serves as the local source of truth.
3.  **Build-Time Injection:** PlatformIO scripts read `config.json` and inject values as C-preprocessor macros (e.g., `LPAD_WIFI_SSID`).
4.  **Graceful Degradation:** If `config.json` is missing or keys are omitted, the system MUST fallback to "Demo Mode" without crashing.

### C. Connectivity Architecture
1.  **Async-First Networking:** `hal_network_init()` initiates the connection process but returns immediately.
2.  **State Polling:** The application monitors connection state via `hal_network_get_status()` in the main loop. Blocking loops (e.g., `while(status != CONNECTED)`) are strictly prohibited in the main thread to prevent UI freezing.
3.  **Domain Abstraction:** HTTP operations use a dedicated `hal_http_client` contract to decouple logic from the specific networking library (Arduino WiFiClient vs ESP-IDF).

### D. Data Flow
1.  **Unidirectional:** Data flows from Source -> Parser -> Model -> View.
    *   `YahooChartParser` (Source) -> `BondTracker` (Model) -> `TimeSeriesGraph` (View) -> `Display` (HAL).
2.  **Immutability:** Parsed data should be treated as immutable once passed to the visualization layer.

### E. Display Architecture
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

### F. State Management
1.  **The Ticker:** The `AnimationTicker` is the single source of truth for time.
2.  **Frame Rate:** The system targets a stable 30fps.
3.  **Delta Time:** All animations (e.g., pulsing indicator) must be calculated based on `deltaTime` provided by the ticker, never on raw frame counts.

### G. Persistent HIL Tests & Demos
1.  **Isolate Demo Code:** To preserve valuable Hardware-in-the-Loop (HIL) test code and release-specific demonstrations, it MUST NOT be written in `src/main.cpp`. A dedicated `demos/` directory will house these runnable examples.
2.  **Naming Convention:** Each demo should correspond to a feature and be named appropriately, e.g., `demos/demo_screen.cpp`. Release demos follow a `demos/demo_release_<version>.cpp` convention.
3.  **Activation via Build Environment:** Demos are compiled and run using dedicated build environments in `platformio.ini`. These environments MUST use the `build_src_filter` option to exclude `main.cpp` (`-<main.cpp>`) and include the target demo file (`+<../demos/demo_name.cpp>`).
4.  **Self-Contained:** Each demo file must be self-contained and provide its own `setup()` and `loop()` functions.
5.  **Development Milestone Policy:** During active feature development, the primary build environments (`esp32s3`, `tdisplay_s3_plus`) MAY reference demo code directly. This allows `pio run -e <board>` to execute the latest working demo without requiring separate demo-specific environment names. Once the application reaches production maturity, these environments should be updated to reference the production `main.cpp` instead.

## 4. Design System & Theming

### A. Theme Architecture
1.  **Isolation:** Themes are stored in `src/themes/<theme_name>/`.
2.  **Manifest Pattern:** Every theme MUST provide a `theme_manifest.h` which acts as the entry point.
3.  **Semantic Mapping:** Code MUST NOT use hardcoded hex colors. It must use semantic names defined in the theme (e.g., `LPad::THEME_TEXT`).

### B. Typography & Fonts
1.  **GFX Fonts:** We use Adafruit GFX compatible header-based fonts.
2.  **Standard Levels:**
    *   `FONT_SMALLEST` (9pt Mono): Ticks and dense labels.
    *   `FONT_NORMAL` (12pt Sans): Paragraphs and body text.
    *   **UI/Axis** (18pt Mono): Axis labels and button text.
    *   **Heading** (24pt Sans): Section headers.
    *   **Title** (48pt Branding): Logo and splash screens.
3.  **Font Conversion:**
    *   Fonts are converted from `.ttf` using the `fontconvert` tool.
    *   A automated script `scripts/generate_theme_fonts.sh` manages this process.
    *   New fonts must be registered in the `theme_manifest.h` using the `extern` pattern to keep the manifest lightweight.

### C. Color Palette
1.  **RGB565:** Microcontroller display operations use 16-bit RGB565.
2.  **RGB888:** Documentation and web-facing tools use 24-bit RGB888.
3.  **Semantic Definitions:** Colors are mapped from a base palette to semantic roles (e.g., `COLOR_SAGE` -> `THEME_PRIMARY`).

## 5. Asset Management

### A. Vector-First Strategy
To ensure resolution independence and maximize memory efficiency on memory-constrained hardware (ESP32-S3), the system prioritizes **Vector Assets** (SVG) over Bitmaps for UI elements (logos, icons, illustrations).

1.  **Memory Efficiency:** Vector meshes occupy significantly less Flash and RAM than pre-rendered bitmaps, especially for high-resolution displays.
2.  **Resolution Independence:** Vector assets scale perfectly to any display size or orientation without aliasing or artifacts.
3.  **CPU over RAM:** We leverage the ESP32-S3's dual-core 240MHz CPU to calculate vertex transformations and fill polygons in real-time, preserving precious PSRAM/SRAM for data buffers.

### B. Asset Pipeline
1.  **Source:** All assets originate as `.svg` files in the `assets/` directory.
2.  **Compilation:** A specialized script (`scripts/process_svgs.py`) parses SVGs and triangulates paths into C++ vertex meshes.
3.  **Static Data:** The resulting meshes are stored as `const` arrays in Flash memory (using the `PROGMEM` pattern where applicable) to minimize runtime heap usage.

### C. Scaling and Animation
1.  **Pivot-Based Transforms:** Rendering supports arbitrary anchor points (0.0 to 1.0) within the asset to facilitate complex scaling (e.g., "scale from center") and translation.
2.  **Interpolation:** Asset properties (X, Y, Scale, Rotation) are interpolated using `deltaTime` to ensure smooth animation at the target frame rate.
3.  **Aspect Ratio Integrity:** Vector assets MUST maintain their original proportions during rendering. The renderer must calculate the target height based on the target width and the asset's intrinsic aspect ratio, regardless of the physical screen's aspect ratio.

## 3. Implementation Patterns

### HAL Implementation
*   **DMA Mandate:** All display drivers MUST use DMA (Direct Memory Access) for pixel transfers where hardware supports it.
*   **Blocking vs. Non-Blocking:** Drawing commands are generally blocking until data is *sent* to the DMA buffer, but return before the transfer is complete on the wire. `hal_display_flush()` acts as the synchronization barrier.

### Feature Flagging
*   **`platformio.ini` as Source of Truth:** Hardware targeting is controlled entirely by build flags in `platformio.ini`. Code uses `#ifdef` guards based on these flags to select the active HAL implementation at compile time.
