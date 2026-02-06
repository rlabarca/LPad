> Prerequisite: features/ui_vector_assets.md
> Prerequisite: features/app_animation_ticker.md

# Feature: Logo Splash Screen

> Label: "Logo Splash Screen"
> Category: "Application Layer"

## Introduction

This feature defines the `LogoScreen` component, which manages the startup animation of the application. It displays the product logo and orchestrates a transition from a large, centered introduction state to a small, unobtrusive corner indicator.

## Component Definition

The `LogoScreen` class should manage its own state and drawing.

### State Machine
1.  **FADE_IN / STATIC:** (Optional) Logo appears.
2.  **WAIT:** Logo remains static in the center for a set duration (2 seconds).
3.  **ANIMATE:** Logo smoothly interpolates position and scale from Center -> TopRight.
4.  **DONE:** Animation is complete.

### Animation Parameters
-   **Start State:**
    -   Position: Center of screen (50%, 50%).
    -   Anchor: Center (0.5, 0.5).
    -   Size: 75% of screen height (approx).
-   **End State:**
    -   Position: Top Right of screen (100%, 0%).
    -   Anchor: Top Right (1.0, 0.0). (Wait, if we anchor TopRight at 1.0,0.0 and place it at Screen 100%,0%, it fits perfectly in the corner).
    -   Size: 5% of screen height.
-   **Duration:** TBD (e.g., 1.5 seconds).
-   **Easing:** Smooth acceleration/deceleration (EaseInOut).

## Implementation Details

### `LogoScreen` Class
-   **Method `init()`:** Sets up initial state.
-   **Method `update(float dt)`:**
    -   Increments internal timers.
    -   Updates position/scale values based on the state.
    -   Calculates interpolation factor `t` (0.0 to 1.0) during the ANIMATE phase.
    -   Returns a status indicating if the screen is "Active" or "Finished" (to allow the demo loop to switch to the graph).
-   **Method `draw(RelativeDisplay& display)`:**
    -   Clears background (Theme background color).
    -   Calls `VectorRenderer::draw` with the current calculated position, size, and anchor.

## Scenarios

### Scenario: Initial Display
-   **Given** the LogoScreen has just started.
-   **When** `draw` is called.
-   **Then** the `LPadLogo` vector asset should be drawn centered (50%, 50%) with a height of ~75% of the screen.

### Scenario: Animation
-   **Given** the 2-second wait period has elapsed.
-   **When** `update` is called.
-   **Then** the Logo's position should move towards (100%, 0%).
-   **And** the Logo's size should decrease towards 5% height.
-   **And** the Logo's anchor point should transition (or be handled such that the visual movement is smooth).
    -   *Note on Anchor Transition:* Linearly interpolating anchor points can be tricky visually. It might be easier to keep the anchor at (0.5, 0.5) and calculate the target absolute center for the top-right corner, OR interpolate the anchor from 0.5,0.5 to 1.0,0.0. The Builder should choose the smoothest mathematical approach.

### Scenario: Completion
-   **Given** the animation is finished.
-   **When** `update` is called.
-   **Then** the screen should indicate it is "Done" (so the main loop knows to transition to the Graph demo).
