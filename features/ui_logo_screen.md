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
    -   Position: Center of screen (50%, 50%) in RelativeDisplay coordinates.
    -   Anchor: Center (0.5, 0.5).
    -   Size: 75% of screen height.
-   **End State:**
    -   **Target Location:** Top-right corner of the screen with 10px buffer
    -   **In Screen Pixels (origin top-left):** Logo's top-right corner at `(ScreenWidth - 10, 10)`
    -   **Visual Intent:** Logo sits 10 pixels from the right edge and 10 pixels from the TOP edge of the screen
    -   **Anchor:** Top-right corner of logo (1.0, 1.0) - anchor Y=1.0 for top in asset space
    -   **Size:** 10% of screen height
-   **Duration:** 1.5 seconds.
-   **Easing:** Smooth acceleration/deceleration (EaseInOutCubic).

## Implementation Details

### `LogoScreen` Class
-   **Method `init()`:** Sets up initial state.
-   **Method `update(float dt)`:**
    -   Increments internal timers.
    -   Updates position/scale values based on the state.
    -   Calculates interpolation factor `t` (0.0 to 1.0) during the ANIMATE phase.
    -   **Target Position Calculation:**
        -   In screen pixels (Y=0 at top): Logo's top-right corner at `(ScreenWidth - 10, 10)`
        -   In RelativeDisplay (Y=0 at bottom, Y=100 at top): Must convert to place logo at TOP of screen
        -   Anchor (1.0, 1.0) means the anchor point is at the logo's top-right corner
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
-   **Then** the Logo's top-right corner should move towards the TOP-right corner of the screen, 10 pixels from the right edge and 10 pixels from the TOP edge.
-   **And** the Logo's size should decrease towards 10% height.
-   **And** the Logo's anchor point should transition smoothly from center (0.5, 0.5) to top-right (1.0, 1.0).
-   **And** the logo should remain fully visible on screen (not extend above the top edge).

### Scenario: Completion
-   **Given** the animation is finished.
-   **When** `update` is called.
-   **Then** the screen should indicate it is "Done" (so the main loop knows to transition to the Graph demo).
