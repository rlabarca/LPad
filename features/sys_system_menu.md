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
*   **Implementation:** `src/system/system_menu_component.cpp`
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

## Implementation Notes

### [2026-02-11] Direct Display Rendering (No PSRAM Canvas)
The SystemMenu draws directly to the main display GFX (not a PSRAM canvas) to avoid the known GFXfont crash on PSRAM Arduino_Canvas. This is safe because the menu suppresses all other rendering while active.

### [2026-02-11] GFXfont Forward Declaration Conflict
**Problem:** `struct GFXfont;` forward declaration in the header conflicts with C-style `typedef struct { ... } GFXfont;` used by Arduino_GFX and native test mocks.
**Solution:** Use `const void*` for font pointers in the header, cast to `const GFXfont*` in the cpp file. Avoids pulling in GFX headers while remaining type-compatible at ABI level.

### [2026-02-11] Gesture Direction Semantics for Menu Activation
Edge drag direction reports the EDGE where the touch started, not the movement direction:
- `TOUCH_DIR_UP` = started from TOP edge (user swiped DOWN from top) → activates menu
- `TOUCH_DIR_DOWN` = started from BOTTOM edge (user swiped UP from bottom) → closes menu

### [2026-02-12] Semantic Color Defaults (v0.71 Sync)
Constructor defaults for `m_versionColor` and `m_ssidColor` were hardcoded hex (`0x7BEF`, `0xFFFF`). Replaced with semantic constants from `theme_colors.h` (`THEME_TEXT_VERSION`, `THEME_TEXT_STATUS`) per `arch_design_system.md §1`. The caller (`main.cpp`) still overrides these from the active theme's `text_version` and `text_status` fields at runtime.
