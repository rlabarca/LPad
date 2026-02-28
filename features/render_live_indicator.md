# Feature: Live Indicator

> Label: "Render: Live Indicator"
> Category: "Rendering"
> Prerequisite: features/arch_display_pipeline.md
> Prerequisite: features/design_visual_language.md

[TODO]

## 1. Overview

The LiveIndicator is a standalone pulsing radial-gradient dot component used to mark the most recent data point on time-series graphs. It smoothly oscillates between configurable min and max radii using smoothstep easing, drawing a radial gradient from inner to outer color via RelativeDisplay.

---

## 2. Requirements

### 2.1 Animation

- `update(deltaTime)` advances the pulse phase: `phase += dt * (1/pulseDuration_sec) * 2*PI`.
- Phase wraps using `fmod` for continuous cycling.
- `getCurrentRadius()` uses smoothstep easing: `t = (sin(phase)+1)/2`, `factor = t^2*(3-2t)`, `radius = min + (max-min) * factor`.

### 2.2 Drawing

- `draw(x_percent, y_percent)` renders a filled circle with radial gradient at the specified relative coordinates.
- Uses `display_relative_fill_circle_gradient()` with a RadialGradient constructed from the current radius and theme colors.

### 2.3 Theme

- `IndicatorTheme` configures: inner color, outer color, min radius (%), max radius (%), pulse duration (ms).

### 2.4 Reset

- `reset()` sets `pulse_phase` to 0, restarting the animation cycle.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Radius oscillates between min and max

    Given an IndicatorTheme with minRadius=1.0 and maxRadius=3.0
    When update is called repeatedly over a full pulse cycle
    Then getCurrentRadius returns values between 1.0 and 3.0

#### Scenario: Smoothstep easing is applied

    Given the pulse phase is at the midpoint
    When getCurrentRadius is calculated
    Then the result follows the smoothstep curve (not linear)

#### Scenario: Reset restarts animation

    Given the pulse has been running for several seconds
    When reset is called
    Then pulse_phase returns to 0
    And the next draw starts from the beginning of the cycle

### Manual Scenarios (Human Verification Required)

None.
