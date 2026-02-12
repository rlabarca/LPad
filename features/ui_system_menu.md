# Feature: UI System Menu

> Label: "System Menu"
> Category: "UI Framework"
> Prerequisite: features/touch_gesture_engine.md, features/ui_theme_support.md

## 1. Introduction
The System Menu is a global, transient UI layer that provides system-level information (version, connectivity) and navigation. It remains hidden until summoned by a specific gesture and has the highest rendering priority, suppressing all underlying application rendering while active.

## 2. Visual Specification
- **Background:** Solid color defined by `colors.system_menu_background` in the active theme (e.g., solid black in the Default theme).
- **Size:** Full screen (covers all underlying content).
- **Typography:**
    - **Version Information:** Displays "Version <version_number>" in the top-left corner.
        - **Font:** `fonts.smallest` (9pt).
        - **Color:** `colors.text_secondary` (subtle).
    - **Connectivity Information:** Displays the active WiFi AP name in the top-right corner.
        - **Font:** `fonts.normal` (12pt).
        - **Color:** `colors.text_main` (normal).

## 3. Interaction & Animation
- **Activation Gesture:** `EDGE_DRAG: TOP` (swipe down from the top edge).
- **Dismissal Gesture:** `EDGE_DRAG: BOTTOM` (swipe up from the bottom edge).
- **Animation:** 
    - The menu must animate down from the top.
    - **Frame Rate:** 30 FPS.
    - **Speed:** "Quick" (e.g., < 300ms for full transition).
- **State Management:** While the menu is in the "OPEN" or "OPENING" state, it must capture all touch events, preventing them from reaching the underlying application layers (e.g., the graph).

## 4. Global Rendering Control
To prevent "fighting" for the display buffer and eliminate screen flashing:
- A global rendering lock or state must be established.
- When the System Menu is active (Opening, Open, or Closing), the `AppCoordinator` or equivalent must suppress the `update()` and `render()` cycles of all other background components (e.g., `V060DemoApp`, `TimeSeriesGraph`).
- The System Menu has exclusive access to the `hal_display_fast_blit` functions while visible.

## 5. Scenarios

### Scenario: Summoning the Menu
- Given the user is viewing the Stock Tracker graph
- When the user performs an `EDGE_DRAG: TOP` gesture
- Then the System Menu begins animating down from the top at 30 FPS
- And the background graph stops updating its display to prevent overlap.

### Scenario: Viewing System Information
- Given the System Menu is fully open
- Then the top-left corner shows the current version in a subtle, small font
- And the top-right corner shows the current WiFi SSID in a normal-sized font.

### Scenario: Dismissing the Menu
- Given the System Menu is open
- When the user performs an `EDGE_DRAG: BOTTOM` gesture
- Then the System Menu animates upwards and disappears
- And once fully closed, the underlying application rendering resumes.
