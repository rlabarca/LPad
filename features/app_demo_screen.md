# Feature: Base UI Demo Application

> Label: "Base UI Demo App"
> Category: "Application Layer"

> **Prerequisite:** `features/app_animation_ticker.md`
> **Prerequisite:** `features/data_yahoo_chart_parser.md`
> **Prerequisite:** `features/ui_time_series_graph.md`
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

## Implementation Notes

- **Separation of Concerns:** The application logic (`main.cpp`) should only coordinate these components. It should not contain raw drawing code.
- **Data Binding:** The `LiveIndicator`'s position must be updated every frame (or whenever data changes) to match the screen coordinates of the newest point on the graph.
- **Performance:** Ensure the full-screen redraw and blit can occur within the 33ms frame budget (for 30fps).
