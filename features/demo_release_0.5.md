# Feature: Release 0.5 Demo Application

> Label: "Demo for Release v0.5"
> Category: "Release Demos"

> **Prerequisite:** `features/app_animation_ticker.md`
> **Prerequisite:** `features/ui_live_indicator.md`
> **Prerequisite:** `features/ui_base.md`
> **Prerequisite:** `features/display_canvas_drawing.md`
> **Prerequisite:** `features/ui_theme_support.md`
> **Prerequisite:** `features/ui_logo_screen.md`

This feature defines the main demo application used to validate Release v0.5. To ensure maintainability and reusability for future releases, the demo logic MUST be encapsulated in a class named `V05DemoApp`.

## Architecture: `V05DemoApp` Class

The `V05DemoApp` class manages the lifecycle and orchestration of the v0.5 visual components.

### Public Interface:
1.  **`begin(RelativeDisplay* display)`**:
    -   Initializes all UI components (`LogoScreen`, `TimeSeriesGraph`, etc.).
    -   Loads the default theme and test data.
    -   Resets the state machine to the initial stage.
2.  **`update(float deltaTime)`**:
    -   Advances the internal state machine through:
        - Stage 0: Logo Animation
        - Stage 1: Graph Mode Cycling (6 modes × 5 seconds each)
    -   Updates active animations (Logo, Live Indicator).
3.  **`render()`**:
    -   Handles the layered rendering of the background, current graph mode, and global title.
4.  **`isFinished()`**: Returns `true` after all 6 graph modes complete.

## Scenario: Application Startup

**Given** the device has booted up and the HAL is initialized.
**When** the demo application starts (`begin` is called).
**Then** the following resources should be initialized:
1.  **Theme:** The default theme is loaded via `ThemeManager`.
2.  **AnimationTicker:** Configured for 30 FPS.
3.  **Graph Data:** Loaded from `test_data/yahoo_chart_tnx_5m_1d.json`.
4.  **Components:**
    -   `LogoScreen`: Initialized and ready to play.
    -   `BackgroundDrawer`: Configured to use theme colors.
    -   `TimeSeriesGraph`: Configured with loaded data, referencing theme fonts for labels.
    -   `LiveIndicator`: Configured to use theme colors for its pulsing animation.

## Scenario: Visual Requirements & Rendering (Demo Loop)

**Given** the application is running.
**When** the loop executes (`update` and `render` are called).
**Then** the display should cycle through the following stages:

### Stage 0: Logo Animation
1.  **Display:**
    -   Solid background (Theme Background Color).
    -   LPad Logo starts centered and large (~75% height).
2.  **Behavior:**
    -   Waits for 2 seconds.
    -   Animates smoothly (EaseInOut) to the top-right corner, shrinking to ~5% height.
    -   Holds the final position for 2 seconds.
    -   Transitions to Stage 1.

### Stage 1: Graph Mode Cycling
The demo cycles through **6 graph modes** (5 seconds each), testing all combinations of:
- **2 Layout Modes:** Scientific (OUTSIDE labels with axis titles) vs. Compact (INSIDE labels, no titles)
- **3 Visual Themes:** Gradient, Solid, Mixed

#### Mode Matrix (2 layouts × 3 themes = 6 modes):

| Mode | Layout     | Theme    | Duration |
|------|------------|----------|----------|
| 0    | Scientific | Gradient | 5s       |
| 1    | Scientific | Solid    | 5s       |
| 2    | Scientific | Mixed    | 5s       |
| 3    | Compact    | Gradient | 5s       |
| 4    | Compact    | Solid    | 5s       |
| 5    | Compact    | Mixed    | 5s       |

After Mode 5 completes, `isFinished()` returns `true`.

#### Layout Modes:

**Scientific Layout (Modes 0-2):**
- **Tick Labels:** `OUTSIDE`
- **Axis Titles:** Enabled
  - X-Axis: "TIME (5m)"
  - Y-Axis: "YIELD (%)" (Vertical, Rotated -90)
- **Constraints:** Graph must auto-scale plotting area to accommodate outside labels/titles. No overlap. First Y-tick label above X-axis line.

**Compact Layout (Modes 3-5):**
- **Tick Labels:** `INSIDE`
- **Axis Titles:** None (Hidden)
- **Constraints:** Plotting area expands to fill available space. Tick labels overlay grid/background.

#### Visual Themes:

**Theme 0: Gradient (ThemeManager colors)**
- Background: 45° gradient from `ThemeColors.background` → `ThemeColors.secondary`
- Line: Horizontal gradient from `ThemeColors.primary` → `ThemeColors.accent`
- Axis: `ThemeColors.secondary`
- Ticks: `ThemeColors.graph_ticks`
- Live Indicator: Radial gradient `ThemeColors.accent` (center) → `ThemeColors.primary` (edge)

**Theme 1: Solid Colors**
- Background: Solid dark grey (0x2104)
- Line: Solid white (RGB565_WHITE)
- Axis: Solid magenta (RGB565_MAGENTA)
- Ticks: Solid cyan (RGB565_CYAN)
- Live Indicator: Solid green (RGB565_GREEN)

**Theme 2: Mixed**
- Background: Solid dark blue (RGB565_DARK_BLUE)
- Line: Horizontal gradient yellow → red
- Axis: Solid cyan (RGB565_CYAN)
- Ticks: Solid white (RGB565_WHITE)
- Live Indicator: Radial gradient yellow → red

### Common Visual Elements (All Graph Modes)
1.  **Title:**
    -   Text: "DEMO v0.5"
    -   **Font:** `ThemeFonts.normal` (12pt).
    -   **Color:** `ThemeColors.text_main`.
    -   Position: Top-left of the screen (approx 5% padding from edges), left-justified, z-index above all other elements.
2.  **Time Series Graph:**
    -   Data: Loaded from embedded `test_data/yahoo_chart_tnx_5m_1d.json`
    -   **Font:** `ThemeFonts.smallest` (9pt) used for all axis text.
    -   **Color:** `ThemeColors.axis_labels` used for tick labels.
    -   Background, line, and indicator styling vary per Visual Theme (see above).
3.  **Live Indicator:**
    -   Position: Exactly at the last data point on the line.
    -   **Animation:** Smooth pulsing (growing/shrinking) at 30fps.
    -   Color styling varies per Visual Theme (see above).

## Hardware (HIL) Test

To visually confirm the correct operation of this feature, the `src/main.cpp` file must be temporarily modified to implement the demo logic using the `V05DemoApp` class.

**Instructions for the Builder:**

1.  **Implement `V05DemoApp`:** Create `demos/v05_demo_app.h` and `.cpp`.
2.  **Modify `src/main.cpp`:** Instantiate and use `V05DemoApp` directly.
3.  **Theme Usage:** explicitly use the `ThemeManager` singleton to retrieve colors and fonts. Do not hardcode hex values.
4.  **Verify:** Compile and upload the firmware. The resulting display must match the Visual Requirements scenario above.
