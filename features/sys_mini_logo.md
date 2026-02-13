# Feature: System Component - Mini Logo

> Label: "System Component: Mini Logo"
> Category: "System Components"
> Prerequisite: features/core_ui_render_manager.md
> Prerequisite: features/ui_mini_logo.md

## 1. Overview
Adapt the existing `UIMiniLogo` (from `features/ui_mini_logo.md`) to function as a registered `SystemComponent` within the new `UIRenderManager` architecture.

## 2. Configuration
*   **Type:** SystemComponent
*   **Z-Order:** 10
*   **Default Visibility:** TRUE (Always visible — `m_visible` defaults to `true` in UIComponent base class)
*   **Activation Event:** None (Passive component)

## 3. Behavior

### 3.1 Lifecycle
*   **Implementation:** `src/system/mini_logo_component.cpp`
*   **Init:** Component is constructed and `begin(RelativeDisplay*)` is called by `main.cpp` before registration. Loads the vector logo asset via the inner `MiniLogo` class.
*   **Visibility:** Starts visible immediately after registration. Unlike activated SystemComponents, `onRun()` is never called — visibility relies on the UIComponent default (`m_visible = true`).
*   **Render:**
    *   Uses `hal_display_fast_blit_transparent` (Chroma Key 0x0001) to draw the logo at the top-right corner.
    *   Adheres to the 30fps animation ticker (if animated).
*   **Pause/Unpause:**
    *   If the system is paused (e.g. by full screen menu), the logo stops updating its animation state but retains its last visual state.

### 3.2 Interaction
*   **Input:** Returns `false` for all input events (Pass-through). It does not block touches.

## 4. Scenarios

### Scenario: Passive Rendering
    Given the MiniLogo is running
    And the current App is "StockTicker"
    When the RenderManager updates
    Then the StockTicker draws first
    And the MiniLogo draws second using Transparent Blit
    And the Logo appears overlaid on the App content

### Scenario: Hidden by System Menu
    Given the MiniLogo is running
    When the System Menu (Z=20) becomes active and Opaque
    Then the MiniLogo `render()` method is NOT called (Occlusion Optimization)
