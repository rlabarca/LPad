# Feature: UI System Menu

> Label: "System Menu"
> Category: "UI Framework"
> Prerequisite: features/touch_gesture_engine.md, features/ui_theme_support.md

## 1. Introduction
The System Menu is a global, transient UI layer that provides system-level information (version, connectivity) and navigation. It remains hidden until summoned by a specific gesture and has the highest rendering priority, suppressing all underlying application rendering while active.

## 2. Visual Specification (Widget-Based)
The System Menu layout is managed by a `WidgetLayoutEngine` using a `GridWidgetLayout`.

- **Background:** Solid color defined by `colors.system_menu_background`.
- **Layout Configuration:**
    - **Type:** `GridWidgetLayout` (1 column x 5 rows).
    - **Position:** Anchored at `TOP_CENTER`, 10% offset down from screen `TOP_CENTER`.
    - **Size:** 50% screen width, 50% screen height.
- **Widgets:**
    1. **Heading (Row 0):** `TextWidget`
        - **Text:** "WiFi Networks"
        - **Font:** `fonts.ui` (18pt)
        - **Color:** `colors.text_heading` (Cream/Bright)
        - **Justification:** `CENTER`, `CENTER`
    2. **WiFi List (Rows 1-4):** `WiFiListWidget`
        - **Content:** List of available pre-configured APs.
        - **Font:** `fonts.normal`
        - **Color (Normal):** `colors.text_main` (Khaki)
        - **Color (Active):** `colors.text_highlight` (Chamoisee)
        - **Behavior:** See `features/ui_wifi_list_widget.md`.

- **Legacy Status Items (Overlay):**
    - The active WiFi SSID and Version number remain in their corners (Top-Right and Bottom-Center) as independent elements or could be migrated to a separate layout in the future. For v0.72, they remain as is, but the SSID must update when selection changes.

## 3. Interaction & Animation
- **Activation Gesture:** `EDGE_DRAG: TOP` (swipe down from the top edge).
- **Dismissal Gesture:** `EDGE_DRAG: BOTTOM` (swipe up from the bottom edge).
- **Animation (Window Shade):** 
    - The menu background MUST animate as a clean "window shade" sliding from the top.
    - **Widget Visibility:** To ensure a clean transition, NO widgets or layout elements should be drawn while the menu is in the "Opening" or "Closing" animation states.
    - **Summoning:** The background slides down; widgets only appear once the menu has reached its fully "OPEN" state.
    - **Dismissing:** Widgets MUST be hidden immediately before the background begins its slide back up to the top.
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
