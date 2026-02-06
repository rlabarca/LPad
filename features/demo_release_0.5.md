# Feature: Release 0.5 Demo Application

> Label: "Demo for Release v0.5"
> Category: "Release Demos"

> **Prerequisite:** `features/app_animation_ticker.md`
> **Prerequisite:** `features/ui_live_indicator.md`
> **Prerequisite:** `features/ui_base.md`
> **Prerequisite:** `features/display_canvas_drawing.md`
> **Prerequisite:** `features/ui_theme_support.md`
> **Prerequisite:** `features/ui_logo_screen.md`

This feature defines the main demo application used to validate Release v0.5. It orchestrates all UI components to showcase the system's capabilities using the active theme.

## Scenario: Application Startup

**Given** the device has booted up and the HAL is initialized.
**When** the demo application starts.
**Then** the following resources should be initialized:
1.  **Theme:** The default theme is loaded via `ThemeManager`.
2.  **AnimationTicker:** Configured for 30 FPS.
3.  **Off-screen Canvas:** `main_canvas` sized to the full display resolution.
4.  **Graph Data:** Loaded from `test_data/yahoo_chart_tnx_5m_1d.json`.
5.  **Components:**
    -   `LogoScreen`: Initialized and ready to play.
    -   `BackgroundDrawer`: Configured to use theme colors.
    -   `TimeSeriesGraph`: Configured with loaded data, referencing theme fonts for labels.
    -   `LiveIndicator`: Configured to use theme colors for its pulsing animation.

## Scenario: Visual Requirements & Rendering (Demo Loop)

**Given** the application is running.
**When** the loop executes.
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

### Stage 1: "Scientific Mode" (Graph)
(Duration: 5 seconds)
1.  **Graph Configuration:**
    -   **Tick Labels:** `OUTSIDE`.


    -   **Axis Titles:** Enabled.

        -   X-Axis: "TIME (5m)"

        -   Y-Axis: "YIELD (%)" (Vertical, Rotated -90).

2.  **Layout Constraints:**

    -   The graph MUST automatically scale its internal plotting area to accommodate the outside labels and titles within the display bounds.

    -   **No Overlap:** Axis titles and tick labels must not overlap.

    -   **Y-Tick Start:** The first visible Y-tick label must be above the X-axis line.



### Stage 2: "Compact Mode" (Minimalist)

1.  **Graph Configuration:**

    -   **Tick Labels:** `INSIDE`.

    -   **Axis Titles:** None (Hidden).

2.  **Layout Constraints:**

    -   The plotting area should expand to fill more of the available space since margins are not needed for outside text.

    -   Tick labels are drawn overlaid on the grid/background.



### Common Visual Elements (All Stages)

1.  **Gradient Background:** 

    -   Draws a 45-degree angled gradient.

    -   **Colors:** Transitions from `ThemeColors.background` (Top-Left) to `ThemeColors.secondary` (Bottom-Right).

    

2.  **Title:**

    -   Text: "V0.5 DEMO"

    -   **Font:** `ThemeFonts.heading` (24pt).

    -   **Color:** `ThemeColors.text_main`.

    -   Position: Top-center of the screen, z-index above all other elements.



3.  **Time Series Graph:**

    -   **Plot Line:** Draws a gradient line.

        -   **Colors:** `ThemeColors.primary` (Oldest data/Left) -> `ThemeColors.accent` (Newest data/Right).

    -   **Font:** `ThemeFonts.smallest` (9pt) used for all axis text.

    -   **Color:** `ThemeColors.axis_labels` used for all axis text.



4.  **Live Indicator:**

    -   Position: Exactly at the last data point on the line.

    -   **Animation:** Smooth pulsing (growing/shrinking) at 30fps.

    -   **Colors:** Radial gradient from `ThemeColors.accent` (Center) to `ThemeColors.primary` (Outer edge).



## Hardware (HIL) Test



To visually confirm the correct operation of this feature, the `src/main.cpp` file must be temporarily modified to implement the demo logic.



**Instructions for the Builder:**



1.  **Modify `src/main.cpp`:** Implement the full demo logic directly within `src/main.cpp`, including the 5-second state machine.

2.  **Theme Usage:** explicitly use the `ThemeManager` singleton to retrieve colors and fonts. Do not hardcode hex values.

3.  **Verify:** Compile and upload the firmware. The resulting display must match the Visual Requirements scenario above, adhering to the "Earth/Green" tones of the default theme (Night/Forest/Sage).
