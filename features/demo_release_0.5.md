# Feature: Release 0.5 Demo Application

> Label: "Demo for Release v0.5"
> Category: "Release Demos"

> **Prerequisite:** `features/app_animation_ticker.md`
> **Prerequisite:** `features/ui_live_indicator.md`
> **Prerequisite:** `features/ui_base.md`
> **Prerequisite:** `features/display_canvas_drawing.md`
> **Prerequisite:** `features/ui_theme_support.md`

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
    -   `BackgroundDrawer`: Configured to use theme colors.
    -   `TimeSeriesGraph`: Configured with loaded data, referencing theme fonts for labels.
    -   `LiveIndicator`: Configured to use theme colors for its pulsing animation.

## Scenario: Visual Requirements & Rendering

**Given** the application is running.
**When** the loop executes.
**Then** the display should render the following layered elements:

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
    -   **Y-Axis Labels:** Visible on the left or right side.
        -   **Font:** `ThemeFonts.smallest` (9pt).
        -   **Color:** `ThemeColors.axis_labels`.

4.  **Live Indicator:**
    -   Position: Exactly at the last data point on the line.
    -   **Animation:** Smooth pulsing (growing/shrinking) at 30fps.
    -   **Colors:** Radial gradient from `ThemeColors.accent` (Center) to `ThemeColors.primary` (Outer edge).

## Hardware (HIL) Test

To visually confirm the correct operation of this feature, the `src/main.cpp` file must be temporarily modified to implement the demo logic.

**Instructions for the Builder:**

1.  **Modify `src/main.cpp`:** Implement the full demo logic directly within `src/main.cpp`.
2.  **Theme Usage:** explicitly use the `ThemeManager` singleton to retrieve colors and fonts. Do not hardcode hex values.
3.  **Verify:** Compile and upload the firmware. The resulting display must match the Visual Requirements scenario above, adhering to the "Earth/Green" tones of the default theme (Night/Forest/Sage).