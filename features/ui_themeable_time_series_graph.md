# Feature: High-Performance Themeable Time Series Graph

> Label: "Themeable Graph"
> Category: "UI Framework"

> **Prerequisites:** 
> - `features/ui_time_series_graph.md`
> - `features/display_relative_drawing.md` (Object-Oriented version)
> - `features/hal_dma_blitting.md`
> - `features/ui_theme_support.md`

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

- **`drawBackground()`**: This method renders static elements (axes, gridlines, background gradients) to the `bg_canvas`. **It MUST retrieve colors and fonts from `ThemeManager::getInstance().getTheme()`**. It should be called only once or when the theme changes.
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