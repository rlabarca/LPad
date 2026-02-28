# Feature: Relative Display

> Label: "UI: Relative Display"
> Category: "UI Framework"
> Prerequisite: features/arch_display_pipeline.md

[TODO]

## 1. Overview

RelativeDisplay provides resolution-independent drawing by mapping floating-point percentage coordinates (0.0-100.0) to absolute pixel positions. It wraps an Arduino_GFX instance and supports primitive drawing (pixels, lines, rectangles), solid and gradient backgrounds (2-color and 3-color linear, radial), and thick line rendering. A backward-compatible procedural API is maintained alongside the OOP class interface.

---

## 2. Requirements

### 2.1 Coordinate Mapping

- `relativeToAbsoluteX(x_percent)` MUST return `round(x/100 * width)`.
- Same formula for Y, Width, and Height conversions.
- All drawing operations accept coordinates in 0.0-100.0 range.

### 2.2 Drawing Primitives

- `drawPixel(x%, y%, color)`, `drawHorizontalLine(y%, x_start%, x_end%, color)`, `drawVerticalLine(x%, y_start%, y_end%, color)`, `fillRect(x%, y%, w%, h%, color)`.
- Horizontal and vertical lines auto-swap start/end if start > end.

### 2.3 Background Fills

- `drawSolidBackground(color)`: fills entire screen.
- `drawGradientBackground(A, B, angle_deg)`: 2-color linear gradient.
- `drawGradientBackground(A, B, C, angle_deg)`: 3-color linear gradient (A->B at t<0.5, B->C at t>=0.5).
- Optimized paths for angle 0 (horizontal) and angle 90 (vertical). General angles use per-pixel dot-product projection.

### 2.4 Gradient Support

- `LinearGradient`: angle, 2-3 color stops.
- `RadialGradient`: center position, radius, inner/outer colors.
- `display_relative_fill_circle_gradient()`: renders a radial gradient circle.

### 2.5 Backward Compatibility

- Procedural API (`display_relative_*` functions) wraps global state initialized by `display_relative_init()`.
- New code SHOULD use the OOP `RelativeDisplay` class directly.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Coordinate mapping at 50 percent

    Given a RelativeDisplay of 400x400 pixels
    When relativeToAbsoluteX(50.0) is called
    Then it returns 200

#### Scenario: Coordinate mapping at boundaries

    Given a RelativeDisplay of 368x448 pixels
    When relativeToAbsoluteX(0.0) is called
    Then it returns 0
    And relativeToAbsoluteX(100.0) returns 368

#### Scenario: Line auto-swaps reversed endpoints

    Given coordinates where start > end
    When drawHorizontalLine(50, 80, 20, color) is called
    Then a line is drawn from 20% to 80% (auto-swapped)

#### Scenario: Gradient background renders without crash

    Given a RelativeDisplay instance
    When drawGradientBackground is called with a 45-degree 2-color gradient
    Then the background renders (per-pixel projection path used)

### Manual Scenarios (Human Verification Required)

None.
