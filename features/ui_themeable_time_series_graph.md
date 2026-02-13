# Feature: High-Performance Themeable Time Series Graph

> Label: "Themeable Graph"
> Category: "UI Framework"

> Prerequisite: features/display_relative_drawing.md
> Prerequisite: features/hal_dma_blitting.md
> Prerequisite: features/ui_theme_support.md
> Prerequisite: features/ui_base.md

This feature refactors the `TimeSeriesGraph` to use a high-performance, layered rendering architecture. It uses multiple off-screen canvases, wrapped by the `RelativeDisplay` class, to eliminate flicker and allow for smooth 30fps animations, while fully supporting dynamic theming.

---

## Part 1: Layered Rendering Architecture

The `TimeSeriesGraph` component will manage three distinct drawing surfaces to achieve its performance goals.

1.  **Background Canvas (`bg_canvas`):** An off-screen canvas stored in **PSRAM**. It holds the static background elements, such as gradients and axis lines, which are rendered only once.
2.  **Data Canvas (`data_canvas`):** A second off-screen canvas, also in **PSRAM**, with a transparent background. It holds the chart's data line, which is redrawn only when the underlying dataset changes.
3.  **Main Display:** The final, visible hardware display.

Each of these surfaces will be managed via a `RelativeDisplay` instance, allowing the same relative drawing code to target any layer.

---

## Part 2: Component Implementation

### `TimeSeriesGraph` Class Structure

The class will be updated to include:
- A `RelativeDisplay` instance for `bg_canvas`.
- A `RelativeDisplay` instance for `data_canvas`.
- Pointers to the underlying `GfxCanvas16` objects for memory management.
- The `draw()` method is replaced by `drawBackground()`, `drawData()`, and a new `render()` method.
- The `update(float deltaTime)` method is retained for animations.

### Memory Management
**Constraint:** The off-screen `GfxCanvas16` buffers for the background and data layers **MUST** be allocated in PSRAM to conserve internal SRAM. The implementation must check for PSRAM availability.

### API and Behavior

- **`setTickLabelPosition(Position pos)`**: Configures whether tick labels are drawn `INSIDE` or `OUTSIDE` the graph axes.
- **`setXAxisTitle(const char* title)`**: Sets the text for the X-axis title. The title is centered horizontally below the X-axis.
- **`setYAxisTitle(const char* title)`**: Sets the text for the Y-axis title. The title is centered vertically along the Y-axis and **MUST be rendered vertically (rotated -90 degrees)**.
- **`drawBackground()`**: This method renders static elements (axes, gridlines, background gradients) to the `bg_canvas`. **It uses the locally stored `GraphTheme` struct** (populated by the caller via `setTheme()`). It should be called only once or when the theme changes.
    - **Auto-Layout:** The component MUST autonomously calculate the internal "content area" (where the line is plotted) by measuring the required space for axis titles and tick labels. It must dynamically adjust its internal margins to ensure axes are sufficiently spaced away from labels.
    - **Collision Avoidance:** The component MUST ensure that axis titles, tick labels, and axis lines do not overlap. It must calculate label bounding boxes and adjust placement (or suppress labels) to maintain clear separation.
    - **Axis Clearance:** No label (X or Y) shall be drawn such that its bounding box overlaps any axis line. The component MUST maintain a consistent, autonomous padding distance between the axis line and its respective labels.
    - **Origin Suppression:** To prevent clutter and overlap at the origin (the intersection of X and Y axes), the component MUST NOT draw tick labels at the origin for either axis. The first visible labels should be at the first tick interval away from the origin.
    - **Unique Label Generation:** The component MUST ensure that all generated Y-axis tick labels are unique when formatted to their required significant digits. If a calculated tick increment results in duplicate labels (e.g., due to rounding), the component MUST dynamically adjust the increment or precision to maintain distinct labels for every tick mark.
- **`drawData()`**: This method clears the `data_canvas` to be fully transparent, then draws the current data set (e.g., the line graph) onto it. It is called only when data is updated via `setData()`.
- **`render()`**: This method performs the final composition to the main display. It first blits the `bg_canvas`, then blits the `data_canvas` on top of it. This method is fast and should be called every frame.
- **`update(float deltaTime)`**: This method handles real-time animations. It draws primitives (like the pulsing live indicator) **directly to the main display** *after* `render()` has been called. This ensures the animation is drawn on top of all other layers without requiring any expensive canvas redraws.

---

## Part 3: Scenarios

### Scenario: Initializing the Graph Layers

**Given** a `TimeSeriesGraph` is initialized with specific dimensions.
**When** the component is first drawn.
**Then** it must allocate two `GfxCanvas16` buffers in PSRAM with the specified dimensions.
**And** it must instantiate `RelativeDisplay` wrappers for the background canvas, data canvas, and main display.
**And** the `drawBackground()` method should be called, which renders the axes and background gradient onto the `bg_canvas` using the active theme's colors.

### Scenario: Updating Data

**Given** the graph has been initialized and rendered.
**When** new data is provided via the `setData()` method.
**Then** the `drawData()` method must be called.
**And** `drawData()` must first clear the `data_canvas` to be transparent.
**And** it must then draw the new data line onto the `data_canvas` using its `RelativeDisplay` instance.
**And** the `bg_canvas` must remain unchanged.

### Scenario: Rendering a Full Frame

**Given** the background and data have been drawn to their respective canvases.
**When** the `render()` method is called on the `TimeSeriesGraph`.
**Then** the `hal_display_fast_blit()` function MUST be used to transfer the `bg_canvas`'s buffer to the main display.
**And** the `hal_display_fast_blit()` function MUST be used again to transfer the `data_canvas`'s buffer, overlaying it on the main display.
**And** the implementation must retrieve the raw data pointer from the `GfxCanvas16` object using its `getBuffer()` method.

### Scenario: Animating the Live Indicator

**Given** a fully rendered graph from the `render()` method.
**And** an `AnimationTicker` is driving the main application loop.
**When** the `update(float deltaTime)` method is called on the `TimeSeriesGraph`.
**Then** the pulsing live indicator circle should be drawn directly to the main display at the position of the last data point.
**And** this operation must not modify the `bg_canvas` or `data_canvas`.
**And** the animation should appear smooth and flicker-free, overlaying the static graph elements.

---

## Part 4: Enhanced Styling & Layout (Consolidated from v2)

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
  - **Font:** Built-in bitmap font (Size 3) due to PSRAM canvas limitations (see Implementation Notes).
  - **Color:** `graph_ticks` color from the current theme (caller configured).
- **But** if `setWatermark` has NOT been called (or called with `nullptr`), NO watermark text should be rendered. This ensures backward compatibility with previous demos.

### Scenario: Dynamic Axis Labels

- **Given** a `TimeSeriesGraph` with a data series.
- **When** the data series is updated with new values that change the min/max range.
- **Then** the Y-axis labels are recalculated and redrawn to reflect the new range.
- **And** the X-axis labels are updated to reflect the new timestamps.

### [2026-02-11] Custom GFX Fonts Crash on PSRAM Canvas
**Problem:** Assigning a custom `GFXfont*` (e.g., `fonts.heading`) to an `Arduino_Canvas` allocated in PSRAM causes immediate `TG1WDT_SYS_RST` (watchdog reset) on ESP32-S3.
**Root Cause:** The `Arduino_GFX` library's font rendering path likely has an issue when accessing font data structures while the target buffer is in external RAM.
**Solution/Workaround:** We MUST use the built-in bitmap font (size 1-3) for any text drawn to a PSRAM canvas.
*   `canvas->setFont(nullptr);`
*   `canvas->setTextSize(2);` // For axis labels
*   `canvas->setTextSize(3);` // For watermark
**Impact:** `setXAxisTitle` and `setYAxisTitle` currently use built-in fonts, ignoring the Theme's font settings. The Watermark also uses built-in size 3.

### [2026-02-11] Y-Axis Tick Spacing & Label Uniqueness
**Problem:** Non-uniform Y-axis tick spacing and duplicate labels (e.g., "4.14", "4.14").
**Root Cause:**
1.  **Rounding Errors:** Ticks at exact clean data values (4.134, 4.136...) mapped to fractional pixel positions. `roundf()` caused non-uniform gaps (2px, 2px, 3px...).
2.  **Duplicate Labels:** Tick increment (0.002) was finer than display precision (3 digits), resulting in identical strings.
**Solution:**
1.  **Iterative Tick Skip:** Instead of forcing pixel uniformity (which violates the spec), we increase tick density until all labels are unique. `while (has_duplicates) tick_skip++;`.
2.  **Accept Non-Uniformity:** We prioritize data accuracy (ticks at exact values) over pixel-perfect spacing.

### [2026-02-12] Y-Axis Title Rendering — Rotated -90° via Temp Canvas
**Problem:** Original char-by-char vertical Y-axis title was fragile and misaligned.
**Solution:** Render the Y-axis title text horizontally to a small temporary canvas, then blit it rotated -90° into the background canvas pixel-by-pixel. This produces clean, properly-spaced rotated text using the built-in bitmap font.

### [2026-02-12] INSIDE Mode Missing Left Margin for Y-Axis Title
**Problem:** In INSIDE tick label mode, the Y-axis title ("Value") overlapped the Y-axis line on the ESP32-S3 AMOLED and was barely separated on the T-Display S3 Plus.
**Root Cause:** `getMargins()` INSIDE branch set `m.left = 3.0f` unconditionally — no extra margin for the Y-axis title. The rotated title (~20px wide) rendered at `start_x = 2`, but the Y-axis line sat at only 3% of width (14-16px), causing overlap.
**Fix:** Added `if (y_axis_title_) m.left += 4.0f;` in INSIDE mode (matching the OUTSIDE mode pattern). This pushes the axis to 7% (33-37px), giving ~11-15px clearance between the title and the axis line.

### [2026-02-06] Title Text Rendering (Superseded)
**Note:** The original chroma-key blit approach for titles has been replaced by the temp-canvas rotation method above. Kept for historical reference.
**Original Problem:** Title text disappeared or flashed when graph updated.
**Original Solution:** Pre-render title to a dedicated off-screen buffer (using chroma key 0x0001) and blit it using `hal_display_fast_blit_transparent` immediately after the graph render.

### [2026-02-04] Flicker-Free Rendering
**Strategy:** "Layered Rendering" (Game Engine style).
1.  **Background Canvas (PSRAM):** Static elements (grid, axes). Drawn once.
2.  **Data Canvas (PSRAM):** Dynamic line. Redrawn on data update.
3.  **Composition:** On `update()`, we blit "clean" pixels from BG and Data canvases to restore the area behind the moving dot, then draw the new dot.
**Lesson:** PSRAM is cheap; SPI bandwidth is expensive. Cache layers to avoid recomputing pixels.

### [2026-02-11] getTextBounds() Width Under-Count
**Problem:** Title displayed "DEMO v0.6" instead of "DEMO v0.65" — last character clipped.
**Root Cause:** `Arduino_GFX::getTextBounds()` can undercount the width of the last character's advance by 1-2 pixels.
**Fix:** Add 2 pixels of padding: `m_titleBufferWidth = w + 2;`
**Lesson:** Never trust `getTextBounds()` for exact canvas sizing. Always add 2-4 pixels of width padding.

### [2026-02-11] Arduino_Canvas Creation Pattern
From debugging TouchTestOverlay crashes (Guru Meditation / StoreProhibited / EXCVADDR 0x00000000):
1. **Heap allocate:** `new Arduino_Canvas(width, height, gfx)` — never stack-allocate large canvases
2. **Initialize framebuffer:** `canvas->begin(GFX_SKIP_OUTPUT_BEGIN)` — without this, fillScreen crashes (null framebuffer)
3. **Reuse:** Create once as class member, never recreate per-frame (heap corruption from repeated alloc/free)
4. **Standalone canvases:** Use `nullptr` as parent to avoid `SPI bus already initialized` crash
5. **Clean up once:** `delete canvas;` in destructor only

### [2026-02-09] Transparent Overlay Pattern (Chroma Key)
```cpp
constexpr uint16_t CHROMA_KEY = 0x0001;
canvas->fillScreen(CHROMA_KEY);
canvas->setTextColor(RGB565_WHITE);
canvas->print(text);
hal_display_fast_blit_transparent(x, y, w, h, buffer, CHROMA_KEY);
```
Use chroma key `0x0001` (not `0x0000`/black) for transparent overlays on animated content. Always use ThemeManager for colors — don't hardcode.

### [2026-02-09] Render Loop Optimization — Data Change Detection
**Problem:** Render loop called graph, mini logo, and title blits every frame (30fps), causing flashing.
**Fix:** Track `m_graphInitialRenderDone` flag and last data length. Full render only when data actually changes; live indicator uses efficient dirty-rect updates every frame. Background tasks (StockTracker) require polling — render loop must check for changes and trigger updates.

### [2026-02-06] hal_display_flush() Is a No-Op on Hardware
On ESP32-S3 AMOLED & T-Display S3 Plus, `hal_display_flush()` is a **no-op** — all drawing operations (Arduino_GFX and DMA blits) write directly to display hardware without framebuffer buffering. Never assume buffering exists. Title text must use pre-rendered buffer + transparent DMA blit (~1-2ms) instead of per-frame font rendering (~10-20ms).

### [2026-02-06] Font Injection via GraphTheme Struct
**Problem:** Including `theme_manager.h` in `ui_time_series_graph.cpp` pulled in 5 font data arrays (~100KB+ static data), causing memory pressure and TG1WDT_SYS_RST watchdog crashes during pixel-intensive background rendering.
**Solution:** Fonts passed through `GraphTheme` struct (`tickFont`, `axisTitleFont`). Demo code (which already includes `theme_manager.h`) sets these when creating themes. Graph code never touches ThemeManager directly.

### [2026-02-05] Runtime Theme Switching
Added `setTheme()` to `TimeSeriesGraph` for runtime theme changes without recreating the object. Static layers (background, data) must be explicitly redrawn when theme changes; dynamic layers (live indicator) continue normally. Both gradient and solid rendering modes must be tested — automated cycling is better than manual switching for HIL validation.

### [2026-02-05] Standalone vs Integrated LiveIndicator Trade-Off
The standalone `LiveIndicator` class is implemented and unit-tested but flashes on SPI displays without integrated dirty-rect. The `TimeSeriesGraph` integrated indicator (with composite buffer restoration) is the reference implementation for flicker-free animation. On bandwidth-limited SPI displays, integrated components with tight coupling to the rendering pipeline beat pure component separation.