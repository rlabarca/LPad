# Feature: Animated Live Indicator

> Label: "Animated Live Indicator"
> Category: "UI Framework"

> **Prerequisite:** `features/display_relative_drawing.md`
> **Prerequisite:** `features/app_animation_ticker.md`

This feature defines a reusable UI component that represents a "live" status or a current data point. It renders as a circle with a radial gradient and supports a pulsating animation.

## Data Structures

### `IndicatorTheme`

- `innerColor`: The color at the center of the indicator.
- `outerColor`: The color at the edge of the indicator.
- `minRadius`: The smallest radius during the pulse cycle (relative to screen width/height or absolute pixels).
- `maxRadius`: The largest radius during the pulse cycle.
- `pulseDuration`: Time in milliseconds for a full grow/shrink cycle.

## Scenario: Rendering a Static Indicator

**Given** the `RelativeDisplay` is initialized.
**And** an `IndicatorTheme` is defined with a red inner color and blue outer color.
**When** the `LiveIndicator` is drawn at position (X, Y) with a fixed radius.
**Then** a circle should be drawn centered at (X, Y).
**And** the center pixel should be red.
**And** the edge pixels should be blue.
**And** the colors should interpolate radially.

## Scenario: Animating the Indicator

**Given** a `LiveIndicator` is initialized with a theme (minRadius=5, maxRadius=15).
**When** the `update(deltaTime)` method is called repeatedly over the `pulseDuration`.
**Then** the reported current radius of the indicator should oscillate smoothly between 5 and 15.
**And** the visual drawing should reflect this changing size.

## Implementation Notes

- **Radial Gradient:** This requires a distance calculation from the center `(x - cx)^2 + (y - cy)^2`.
- **Optimization:** Since the indicator is small, per-pixel calculation is acceptable. Alternatively, pre-rendering a texture to a small canvas and scaling/blitting it could be faster, but direct drawing is preferred for quality if performance allows.
- **Math:** Use a sine wave or similar easing function based on accumulated time for the pulse animation to ensure smoothness.
