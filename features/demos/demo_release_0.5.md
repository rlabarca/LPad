# Feature: Release 0.5 Demo Application

> Label: "Demo for Release v0.5"
> Category: "Release Demos"

> **Prerequisite:** `features/app_animation_ticker.md`
> **Prerequisite:** `features/ui_live_indicator.md`
> **Prerequisite:** `features/display_background.md`
> **Prerequisite:** `features/display_canvas_drawing.md`

This feature defines the main demo application. It orchestrates all UI components to showcase the system's capabilities: HAL abstraction, resolution independence, layered rendering, and smooth animation.

## Scenario: Application Startup

**Given** the device has booted up and the HAL is initialized.
**When** the demo application starts.
**Then** the following resources should be initialized:
1.  **AnimationTicker:** Configured for 30 FPS.
2.  **Off-screen Canvas:** `main_canvas` sized to the full display resolution.
3.  **Graph Data:** Loaded from `test_data/yahoo_chart_tnx_5m_1d.json`.
4.  **Components:**
    -   `BackgroundDrawer`: Configured for a 45-degree gradient (Purple -> Pink -> Dark Blue).
    -   `TimeSeriesGraph`: Configured with the "Vaporwave" theme (Cyan->Pink gradient line, ticks enabled).
    -   `LiveIndicator`: Configured with a radial gradient (Pink->Cyan) and pulsing animation.

## Scenario: Application Loop and Rendering

**Given** the application is running.
**When** the loop executes.
**Then** it should wait for the `AnimationTicker` to signal the next frame.
**And** perform the drawing sequence:
1.  **Select Target:** Select `main_canvas` for drawing.
2.  **Draw Background:** Call `drawGradientBackground` to fill the canvas.
3.  **Draw Graph:** Update and draw the `TimeSeriesGraph` on the canvas.
4.  **Draw Indicator:** Update the `LiveIndicator` (pulse animation) and draw it at the position of the last data point.
5.  **Blit to Screen:** Select the main display (nullptr) and draw `main_canvas` to (0,0).

## Hardware (HIL) Test

To visually confirm the correct operation of this feature, a persistent demo must be created, following the rules in `docs/ARCHITECTURE.md`.

**Instructions for the Builder:**

1.  **Create Demo File:** Create a new file at `demos/demo_release_0.5.cpp`.
2.  **Implement Demo Logic:** Implement the full demo logic (as described in the "Application Startup" and "Application Loop" scenarios) inside `demos/demo_release_0.5.cpp`. This file must contain its own `setup()` and `loop()` functions.
3.  **Create Build Environment:** Add a new build environment to `platformio.ini` named `[env:demo_release_0_5]` that is configured to build *only* the `demos/demo_release_0.5.cpp` file.
    *   It should inherit from a common board configuration (if one exists).
    *   It MUST use `src_filter` to exclude `src/main.cpp` and include `demos/demo_release_0.5.cpp`.
    *   Example `src_filter` configuration:
        ```ini
        src_filter =
          -<.git/>, -<svn/>, -<example/>, -<examples/>, -<test/>, -<tests/>
          -<src/>
          +<demos/demo_release_0.5.cpp>
        ```
