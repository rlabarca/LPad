# Feature: UI Render Manager

> Label: "UI: Render Manager"
> Category: "UI Framework"
> Prerequisite: features/arch_component_model.md

[TODO]

## 1. Overview

The UIRenderManager is a singleton that orchestrates all visual rendering and input routing for the LPad UI. It maintains a flat array of up to 16 components sorted by Z-order, renders them using the Painter's Algorithm (back-to-front), dispatches input events in reverse Z-order (front-to-back), and performs occlusion culling to skip rendering of fully hidden components.

---

## 2. Requirements

### 2.1 Component Registration

- `registerComponent(component, zOrder)` MUST return `false` if the component is null, the registry is full (16), or the Z-order collides with an existing component.
- After registration, the internal array MUST be sorted by ascending Z-order.
- For SystemComponents, registration MUST wire the back-pointer (`m_manager`) for `systemPause()` support.

### 2.2 Active App Management

- `setActiveApp(app)` MUST call `onPause()` on the previous active app and `onRun()` on the new active app.
- Only one AppComponent may be active at a time.

### 2.3 Rendering

- `renderAll()` MUST render components in ascending Z-order (Painter's Algorithm: lowest Z first).
- An occlusion floor MUST be calculated: the highest-Z component that is visible, not paused, opaque, and fullscreen. Components below this floor are skipped.
- After rendering, the flush callback (if set) MUST be invoked.

### 2.4 Update

- `updateAll(dt)` MUST call `update(dt)` on all visible, non-paused components.

### 2.5 Input Routing

- `routeInput(event)` MUST first scan all SystemComponents for activation gesture matches. If a paused SystemComponent's activation event matches, the active app is paused and the SystemComponent is shown.
- After activation scanning, events are dispatched in descending Z-order (highest Z first). The first component that returns `true` from `handleInput()` consumes the event.

### 2.6 System Component Pause

- When a SystemComponent calls `systemPause()`, the manager MUST hide the SystemComponent and call `onUnpause()` on the active app.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Register component succeeds

    Given the UIRenderManager has fewer than 16 components
    When registerComponent is called with a valid component and unique Z-order
    Then it returns true
    And getComponentCount increases by 1

#### Scenario: Register rejects null component

    Given the UIRenderManager is available
    When registerComponent is called with a null pointer
    Then it returns false

#### Scenario: Register rejects duplicate Z-order

    Given a component is registered at Z-order 5
    When another component is registered at Z-order 5
    Then it returns false

#### Scenario: Register rejects when registry is full

    Given 16 components are registered
    When a 17th component is registered
    Then it returns false

#### Scenario: Components render in ascending Z-order

    Given components at Z=0, Z=5, Z=10 are registered
    When renderAll is called
    Then components are drawn in order Z=0, Z=5, Z=10

#### Scenario: Occlusion floor skips hidden components

    Given Z=0 and Z=5 are registered
    And Z=5 reports isOpaque=true and isFullscreen=true
    When renderAll is called
    Then Z=0 is not rendered (occluded by Z=5)

#### Scenario: Set active app pauses previous

    Given AppComponent A is the active app
    When setActiveApp(B) is called
    Then A.onPause() is called
    And B.onRun() is called

#### Scenario: Input routed to highest Z first

    Given components at Z=1 and Z=10 are registered and visible
    When routeInput is called with a touch event
    Then Z=10 receives the event before Z=1

#### Scenario: Input consumed by first handler

    Given Z=10 returns true from handleInput
    When routeInput is called
    Then Z=1 does not receive the event

#### Scenario: SystemComponent activation via gesture match

    Given a SystemComponent at Z=20 has activation event EDGE_DRAG UP and is paused
    When routeInput receives an EDGE_DRAG UP event
    Then the active app is paused
    And the SystemComponent is shown via onUnpause

#### Scenario: SystemComponent pause restores active app

    Given a SystemComponent is shown and the active app is paused
    When the SystemComponent calls systemPause
    Then the SystemComponent is hidden
    And the active app receives onUnpause

#### Scenario: Reset clears all state

    Given components are registered
    When reset() is called
    Then getComponentCount returns 0
    And getActiveApp returns null

### Manual Scenarios (Human Verification Required)

None.
