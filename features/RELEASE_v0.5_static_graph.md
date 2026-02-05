# Release v0.5 - Basic Animated Static Test Graph

> Label: "Release v0.5 - Basic Animated Static Test Graph"
> Category: "Releases"

TODO: Trigger recursive validation.

> Prerequisite: features/hal_contracts.md
> Prerequisite: features/hal_timer_esp32.md
> Prerequisite: features/hal_dma_blitting.md
> Prerequisite: features/display_rotation_contract.md
> Prerequisite: features/display_target_rotation.md
> Prerequisite: features/display_esp32s3_amoled.md
> Prerequisite: features/display_tdisplay_s3_plus.md
> Prerequisite: features/display_canvas_drawing.md
> Prerequisite: features/display_relative_drawing.md
> Prerequisite: features/data_yahoo_chart_parser.md
> Prerequisite: features/app_animation_ticker.md
> Prerequisite: features/ui_time_series_graph.md
> Prerequisite: features/ui_themeable_time_series_graph.md
> Prerequisite: features/app_bond_tracker.md

## 1. Release Capability Description
This release marks the completion of the core rendering engine and the first functional application (Bond Tracker). The system is capable of parsing external data, mapping it to a resolution-independent coordinate system, and rendering it with a high-fidelity "Vaporwave" aesthetic using a layered composition strategy to ensure flicker-free 30fps animation.

## 2. Integration Test Criteria
To validate this release, the system must demonstrate the following visual behaviors on the target hardware (LilyGo T-Display S3 AMOLED or T-Display S3 Plus):

### Static Elements
1.  **Gradient Background:** A 45-degree angled gradient background transitioning smoothly from Purple (Top-Left) to Pink to Dark Blue (Bottom-Right).
2.  **Axes:** Pink X and Y axes.
3.  **Ticks:** White tick marks visible on the Y-axis.

### Data Visualization
4.  **Plot Line:** A reasonably thick linear plot connecting data points with smooth corners (no jagged aliasing artifacts where possible).
5.  **Line Gradient:** The plot line itself must exhibit a single horizontal gradient, starting from Cyan at the left (oldest data) and transitioning to Pink at the right (newest data).

### Animation (Dynamic Elements)
6.  **Live Indicator:** A round indicator positioned exactly at the last data point on the line.
7.  **Indicator Styling:** The indicator must be colored with a radial gradient: Pink in the center, fading to Cyan on the outer edge.
8.  **Animation Behavior:** The indicator must smoothly pulsate at 30fps.
    *   **Min Size:** Very small (almost pixel-sized).
    *   **Max Size:** Approximately 10% of the screen width.
    *   **Quality:** The animation must be smooth with no flickering of the underlying graph or background (validating the layered rendering architecture).
