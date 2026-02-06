# Feature: Base UI Elements

> Label: "Base UI Elements"
> Category: "UI Framework"

> **Prerequisite:** `features/display_relative_drawing.md`

This feature defines a utility for filling the display (or a selected canvas) with a background. It supports solid colors and linear gradients with 2 or 3 stops at arbitrary angles.

## API Contract

The functionality should be exposed via a `BackgroundDrawer` class or a set of functions within the `RelativeDisplay` namespace/class. It must work on the currently selected HAL drawing target.

### Supported Fills
1.  **Solid Color:** Fill the entire target with a single 16-bit color.
2.  **Linear Gradient (2 Colors):** Transition from Color A to Color B at a specified angle (0-360 degrees).
3.  **Linear Gradient (3 Colors):** Transition from Color A to Color B to Color C at a specified angle.

## Scenario: Drawing a Solid Background

**Given** the `RelativeDisplay` is initialized.
**When** `drawSolidBackground(0xF800)` (Red) is called.
**Then** the entire drawing area should be filled with red.

## Scenario: Drawing a 2-Color Linear Gradient

**Given** the `RelativeDisplay` is initialized.
**When** `drawGradientBackground(ColorA, ColorB, 45_degrees)` is called.
**Then** the pixel at the top-left corner should match Color A (approx).
**And** the pixel at the bottom-right corner should match Color B (approx).
**And** the pixels in between should transition smoothly.

## Scenario: Drawing a 3-Color Linear Gradient

**Given** the `RelativeDisplay` is initialized.
**When** `drawGradientBackground(ColorA, ColorB, ColorC, 90_degrees)` is called.
**Then** the top of the screen should be Color A.
**And** the middle of the screen should be Color B.
**And** the bottom of the screen should be Color C.

## Implementation Notes

- **Performance:** Gradient calculation for every pixel every frame is expensive.
    -   *Optimization:* For this feature, we assume the background is drawn **once** per frame or dirty rect.
    -   If the background is static, it should be drawn to a cached `Canvas` (see `features/display_canvas_drawing.md`) once, and that canvas should be blitted to the screen each frame. This feature describes the *drawing algorithm*, not necessarily the caching strategy, but the implementation should be efficient enough for initialization.
- **Math:** The gradient logic needs to calculate the interpolation factor `t` (0.0 to 1.0) based on the pixel's projection onto the gradient vector defined by the angle.
