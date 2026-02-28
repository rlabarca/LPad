# Feature: Mini Logo Component

> Label: "System: Mini Logo"
> Category: "System Components"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/arch_display_pipeline.md

[TODO]

## 1. Overview

The MiniLogoComponent is a SystemComponent (Z=10) that renders a persistent transparent vector logo overlay in the top-right corner of the screen. It starts hidden and is shown by the BootLogoApp when the boot animation completes. The component is passive -- it renders the logo each frame but does not handle any input events.

---

## 2. Requirements

### 2.1 Lifecycle

- `begin(display)` creates a MiniLogo instance positioned at `MiniLogo::Corner::TOP_RIGHT`. Returns false if display is null.
- Starts hidden (`hide()` called in main.cpp setup). Shown via `show()` from BootLogoApp DONE state.

### 2.2 Rendering

- `render()` forwards to the internal MiniLogo renderer.
- The logo is drawn as a small vector shape using VectorRenderer with transparency.

### 2.3 Component Properties

- `handleInput()` always returns `false` (input passes through to lower Z components).
- `isOpaque()` returns `false`. `isFullscreen()` returns `false`.
- These properties are critical: returning true for either would break occlusion optimization for components below Z=10.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Begin rejects null display

    Given a MiniLogoComponent instance
    When begin is called with a null display
    Then it returns false

#### Scenario: Component starts hidden

    Given MiniLogoComponent is registered at Z=10
    When setup completes
    Then isVisible returns false

#### Scenario: Input always passes through

    Given the MiniLogoComponent is visible
    When handleInput is called with any event
    Then it returns false

#### Scenario: Not opaque and not fullscreen

    Given the MiniLogoComponent exists
    When isOpaque and isFullscreen are queried
    Then both return false

### Manual Scenarios (Human Verification Required)

#### Scenario: Logo overlay visible after boot

    Given the boot animation has completed
    When the stock chart is displayed
    Then a small LPad logo is visible in the top-right corner
    And the chart renders underneath the logo (transparency)
