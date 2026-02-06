# Feature: Release 0.5 Demo Application

> Label: "Demo for Release v0.5"
> Category: "Release Demos"

> **Prerequisite:** `features/app_animation_ticker.md`
> **Prerequisite:** `features/ui_live_indicator.md`
> **Prerequisite:** `features/ui_base.md`
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
    -   `BackgroundDrawer`: Configured for both a 45-degree gradient (Purple -> Pink -> Dark Blue) AND a solid color (e.g., Dark Grey).
    -   `TimeSeriesGraph`: Configured with hardcoded sample data (e.g., a simple sine wave or flat line) and tested with both a gradient plot line (e.g., Cyan->Pink) AND a solid plot line (e.g., White), with ticks enabled.
    -   `LiveIndicator`: Configured with both a radial gradient (Pink->Cyan) AND a solid color (e.g., Green), and includes pulsing animation.

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

To visually confirm the correct operation of this feature, the `src/main.cpp` file must be temporarily modified to implement the demo logic.

**Instructions for the Builder:**

1.  **Modify `src/main.cpp`:** Implement the full demo logic (as described in the "Application Startup" and "Application Loop" scenarios) directly within `src/main.cpp`. This file must contain its own `setup()` and `loop()` functions.
2.  **Verify:** Compile and upload the firmware to the target hardware using the standard build environment (e.g., `pio run -e esp32s3`).



