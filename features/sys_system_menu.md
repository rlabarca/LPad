# Feature: System Component - System Menu

> Label: "System Component: System Menu"
> Category: "System Components"
> Prerequisite: features/core_ui_render_manager.md
> Prerequisite: features/ui_system_menu.md

## 1. Overview
Adapt the existing `UISystemMenu` (from `features/ui_system_menu.md` and `RELEASE_v0.67`) to function as a registered `SystemComponent`.

## 2. Configuration
*   **Type:** SystemComponent
*   **Z-Order:** 20 (Top Layer)
*   **ShowOnRun:** FALSE (Hidden by default)
*   **Activation Event:** `EDGE_DRAG: TOP`

## 3. Behavior

### 3.1 Lifecycle
*   **Init:** Pre-allocates necessary resources but does NOT draw.
*   **Run:** Stays hidden (`visible = false`).
*   **Activation (UnPause/Show):**
    *   Triggered by `UIRenderManager` when `EDGE_DRAG: TOP` occurs.
    *   Sets `visible = true`.
    *   Reports `isOpaque = true` and `isFullscreen = true` to the Manager.
    *   Draws the menu background and items.
*   **Closing (SystemPause):**
    *   Triggered by `EDGE_DRAG: BOTTOM` (or internal close logic).
    *   Calls `manager->systemPause(this)`.
    *   Sets `visible = false`.

### 3.2 Interaction
*   **Input:**
    *   When Active: Consumes ALL touch events (`return true`).
    *   Implements the swipe-to-close logic (`EDGE_DRAG: BOTTOM`).

## 4. Scenarios

### Scenario: Activation from Background
    Given the SystemMenu is initialized but hidden
    When the User performs an "EDGE_DRAG: TOP" gesture
    Then the RenderManager detects the Activation Event
    And the SystemMenu `onUnpause()` is called
    And the SystemMenu becomes visible
    And the SystemMenu blocks all input to lower layers

### Scenario: Closing the Menu
    Given the SystemMenu is visible
    When the User performs an "EDGE_DRAG: BOTTOM" gesture
    Then the SystemMenu detects the close gesture
    And the SystemMenu calls `systemPause()`
    And the RenderManager calls `onPause()` on the SystemMenu
    And the SystemMenu becomes hidden
    And input control returns to the App
