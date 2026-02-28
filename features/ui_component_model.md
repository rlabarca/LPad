# Feature: UI Component Model

> Label: "UI: Component Model"
> Category: "UI Framework"
> Prerequisite: features/arch_component_model.md

[TODO]

## 1. Overview

The UI component model defines the abstract base class hierarchy for all renderable elements in LPad. `UIComponent` is the abstract root with two concrete subclasses: `AppComponent` for full-screen applications (only one active at a time) and `SystemComponent` for overlay panels with gesture-based activation (can be independently shown/hidden). The model defines lifecycle hooks, visibility/pause state, and Z-order metadata.

---

## 2. Requirements

### 2.1 UIComponent Base Class

- MUST define pure virtual methods: `render()` and `getComponentType()`.
- MUST provide default no-op implementations for: `onRun()`, `onPause()`, `onUnpause()`, `update(dt)`, `handleInput(event)`.
- `isOpaque()` and `isFullscreen()` MUST default to `false`. Subclasses override to enable occlusion optimization.
- `isVisible()` and `isPaused()` MUST track component state via `m_visible` (default true) and `m_paused` (default false).
- `getZOrder()` MUST return the Z-order assigned during registration.

### 2.2 AppComponent

- `getComponentType()` MUST return `Type::APP`.
- MUST provide virtual `onClose()` for cleanup when the app is permanently removed.
- Only one AppComponent is active at a time, enforced by UIRenderManager.

### 2.3 SystemComponent

- `getComponentType()` MUST return `Type::SYSTEM`.
- `show()` MUST set visible=true, paused=false, and call `onUnpause()`.
- `hide()` MUST set visible=false, paused=true, and call `onPause()`.
- `systemPause()` MUST delegate to the UIRenderManager to hide self and resume the active app.
- `setActivationEvent(type, direction)` MUST configure the gesture that triggers automatic show via UIRenderManager input routing.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: AppComponent reports correct type

    Given an AppComponent instance
    When getComponentType is called
    Then it returns Type::APP

#### Scenario: SystemComponent reports correct type

    Given a SystemComponent instance
    When getComponentType is called
    Then it returns Type::SYSTEM

#### Scenario: Default visibility is true

    Given a freshly constructed UIComponent
    When isVisible is called
    Then it returns true

#### Scenario: Default pause state is false

    Given a freshly constructed UIComponent
    When isPaused is called
    Then it returns false

#### Scenario: SystemComponent show sets correct state

    Given a SystemComponent that is hidden and paused
    When show() is called
    Then isVisible returns true
    And isPaused returns false

#### Scenario: SystemComponent hide sets correct state

    Given a SystemComponent that is visible and unpaused
    When hide() is called
    Then isVisible returns false
    And isPaused returns true

#### Scenario: Default opaque and fullscreen are false

    Given a freshly constructed UIComponent
    When isOpaque and isFullscreen are queried
    Then both return false

### Manual Scenarios (Human Verification Required)

None.
