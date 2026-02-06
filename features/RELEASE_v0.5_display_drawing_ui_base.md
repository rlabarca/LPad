# Release v0.5 - Display, Drawing & UI Base

> Label: "Release v0.5 - Display, Drawing & UI Base"
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
> Prerequisite: features/ui_base.md
> Prerequisite: features/app_animation_ticker.md
> Prerequisite: features/ui_live_indicator.md
> Prerequisite: features/demos/demo_release_0.5.md

## 1. Release Capability Description
This release marks the completion of the core rendering engine and the unified "Demo Screen". It validates the system's ability to handle complex UI composition, resolution independence, and smooth 30fps animation across different hardware targets.

## 2. Integration Test Criteria
To validate this release, the system must demonstrate the following visual behaviors on the target hardware (LilyGo T-Display S3 AMOLED or T-Display S3 Plus):

### Static Elements
1.  **Gradient Background:** A 45-degree angled gradient background transitioning smoothly from Purple (Top-Left) to Pink to Dark Blue (Bottom-Right).
2.  **Graph Composition:** A Time Series Graph overlaying the background.
3.  **Axes & Ticks:** Visible X and Y axes with tick marks.

### Data Visualization
4.  **Gradient Plot Line:** The plot line must exhibit a single horizontal gradient, starting from Cyan at the left (oldest data) and transitioning to Pink at the right (newest data).

### Animation (Dynamic Elements)
5.  **Live Indicator:** A round indicator positioned exactly at the last data point on the line.
6.  **Indicator Styling:** The indicator must be colored with a radial gradient: Pink in the center, fading to Cyan on the outer edge.
7.  **Pulse Animation:** The indicator must smoothly pulsate at 30fps (growing and shrinking).
8.  **Smoothness:** The animation must be smooth with no flickering, validating the double-buffering/canvas architecture.