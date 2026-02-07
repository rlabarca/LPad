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

### D. Data Layer Architecture

#### 1. Data Abstraction Hierarchy
```
DataItem (Abstract Base)
  └─ DataItemTimeSeries (FIFO Ring Buffer)
```

1.  **DataItem Contract:** Abstract base class defining common metadata (name, timestamp, value, min/max tracking).
2.  **DataItemTimeSeries:** Specialized subclass implementing a fixed-capacity FIFO ring buffer for time series data.
    *   **Capacity:** Fixed at construction (e.g., 15 points for sliding window)
    *   **FIFO Eviction:** When buffer is full, oldest data point is automatically removed
    *   **Automatic Statistics:** Min/max values updated on every `addDataPoint()` call
    *   **GraphData Export:** `getGraphData()` returns `GraphData` struct compatible with `TimeSeriesGraph`

#### 2. Data Flow Patterns
1.  **Static Data (v0.5/v0.55):**
    *   `YahooChartParser` (Source) -> `GraphData` (Struct) -> `TimeSeriesGraph` (View) -> `Display` (HAL)
2.  **Live Data (v0.58+):**
    *   Embedded Test Data -> `DataItemTimeSeries` (Model) -> `getGraphData()` -> `TimeSeriesGraph` (View) -> `Display` (HAL)
    *   Random Data Injection -> `addDataPoint()` -> FIFO Update -> Graph Refresh
3.  **Immutability:** Parsed data should be treated as immutable once passed to the visualization layer.

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

### G. Demo Architecture (Dispatcher Pattern)

The system uses a **Dispatcher Pattern** for milestone demos, allowing `main.cpp` to point to the active demo while maintaining separate, clearly-labeled entry points for each milestone.

#### Structure
```
src/main.cpp                    # Dispatcher - selects demo via build flags
demos/
  ├─ demo_v05_entry.cpp         # V0.5 entry point (setup/loop)
  ├─ demo_v05_entry.h
  ├─ demo_v055_entry.cpp        # V0.55 entry point (setup/loop)
  ├─ demo_v055_entry.h
  ├─ demo_v058_entry.cpp        # V0.58 entry point (setup/loop)
  ├─ demo_v058_entry.h
  ├─ v05_demo_app.cpp           # V0.5 core logic (Logo + 6 Graph Modes)
  ├─ v05_demo_app.h
  ├─ v055_demo_app.cpp          # V0.55 wrapper (WiFi + V0.5 demo)
  ├─ v055_demo_app.h
  ├─ v058_demo_app.cpp          # V0.58 wrapper (V0.55 + Live Data)
  └─ v058_demo_app.h
```

#### Rules
1.  **main.cpp as Dispatcher:** `main.cpp` MUST NOT contain demo logic. It selects the active entry point via conditional compilation based on build flags (`-DDEMO_V05`, `-DDEMO_V055`, etc.).
2.  **Entry Points:** Each milestone has a dedicated `demo_vXX_entry.cpp` file that implements `demo_setup()` and `demo_loop()` functions.
3.  **Core Logic:** Reusable demo logic lives in `vXX_demo_app.cpp` classes (e.g., `V05DemoApp`). These classes are shared across milestones to maximize code reuse.
4.  **Code Sharing:** Later milestones SHOULD wrap earlier ones when possible. Example: `V055DemoApp` wraps `V05DemoApp`, adding only WiFi connectivity phase.
5.  **Build Flags:** Each demo environment sets a build flag:
    - `demo_v05_esp32s3`: `-DDEMO_V05`
    - `demo_v055_esp32s3`: `-DDEMO_V055`
    - `demo_v058_esp32s3`: `-DDEMO_V058`
    - Base hardware environments (`esp32s3`, `tdisplay_s3_plus`): Use latest demo (currently `-DDEMO_V058`)

#### V0.58 Specific Architecture (Live Data Updates)

**Wrapper Pattern:** V058DemoApp wraps V055DemoApp, which wraps V05DemoApp.

**Data Source:** Embedded test data header (`test_data/test_data_tnx_5m.h`) instead of filesystem, avoiding LittleFS dependency.

**Live Data Injection:**
1. `DataItemTimeSeries` initialized with 15-point capacity (matches test data size)
2. Initial Y-bounds captured from test data (prevents drift over time)
3. New random data point injected every 3 seconds
4. FIFO buffer automatically evicts oldest point, maintaining sliding 15-point window
5. Graph updated via `setData()` + `drawData()` + `render()`

**Phase/Stage Gating:**
To prevent graph from overwriting logo or connectivity screens, updates require:
- V055 Phase: `PHASE_VISUAL_DEMO` (not connectivity or handover)
- V05 Stage: `STAGE_GRAPH_CYCLE` (not logo animation)
- Check via: `v055Demo->isInVisualPhase() && v05Demo->isShowingGraph()`

**Dual Rendering Paths:**
- **Graph:** `TimeSeriesGraph::render()` → `hal_display_fast_blit()` → Direct DMA to display
- **Title:** `V05DemoApp::drawTitle()` → Arduino_GFX framebuffer → Requires `hal_display_flush()`
- **Synchronization:** `flush()` called immediately after `drawTitle()` to sync DMA and framebuffer
- **Known Issue:** Title may still flicker due to async rendering paths (under investigation)

#### Adding a New Milestone Demo
1.  Create `demos/demo_vXX_entry.h` and `.cpp` with `demo_setup()` and `demo_loop()`
2.  Create `demos/vXX_demo_app.h` and `.cpp` if new core logic is needed (or reuse existing)
3.  Add `-DDEMO_VXX` build flag to `platformio.ini` environment
4.  Update `src/main.cpp` conditional: `#elif defined(DEMO_VXX)`
5.  Document in `features/demo_release_X.X.md`

#### Benefits
- **Clear Labeling:** Each milestone demo is in a separate, clearly-named file
- **Code Sharing:** V0.55 wraps V0.5, V0.6 might wrap V0.55, etc.
- **No Duplication:** Shared logic lives in `vXX_demo_app.cpp` classes
- **Easy Navigation:** `main.cpp` points to active milestone
- **Maintainability:** Old demos preserved for regression testing

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
