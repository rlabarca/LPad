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
    -   Position: Top Right of screen (Absolute Top Right), but offset by 10 pixels in both X and Y (moving away from the corner).
    -   Anchor: Top Left of logo (0.0, 0.0).
    -   Size: 10% of screen height.
-   **Duration:** TBD (e.g., 1.5 seconds).
-   **Easing:** Smooth acceleration/deceleration (EaseInOut).

## Implementation Details

### `LogoScreen` Class
-   **Method `init()`:** Sets up initial state.
-   **Method `update(float dt)`:**
    -   Increments internal timers.
    -   Updates position/scale values based on the state.
    -   Calculates interpolation factor `t` (0.0 to 1.0) during the ANIMATE phase.
    -   Target Position Calculation: The endpoint is `(ScreenWidth - 10, 10)` in absolute pixels, using a Top-Left anchor for the logo.
    -   Returns a status indicating if the screen is "Active" or "Finished".
-   **Method `draw(RelativeDisplay& display)`:**
    -   Clears background (Theme background color).
    -   Calls `VectorRenderer::draw` with the current calculated position, size, and anchor.

## Scenarios

### Scenario: Initial Display
-   **Given** the LogoScreen has just started.
-   **When** `draw` is called.
-   **Then** the `LPadLogo` vector asset should be drawn centered (50%, 50%) with a height of ~75% of the screen.

### Scenario: Animation to Corner
-   **Given** the 2-second wait period has elapsed.
-   **When** `update` is called.
-   **Then** the Logo's top-left corner should move towards 10 pixels from the top-right corner of the screen.
-   **And** the Logo's size should decrease towards 10% height.
-   **And** the Logo's anchor point should transition smoothly from center (0.5, 0.5) to top-left (0.0, 0.0).

### Scenario: Completion
-   **Given** the animation is finished.
-   **When** `update` is called.
-   **Then** the screen should indicate it is "Done" (so the main loop knows to transition to the Graph demo).
