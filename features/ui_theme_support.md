# UI Theme Support

> Label: "Theme Support"
> Category: "UI Framework"
> Prerequisite: features/display_canvas_drawing.md
> Prerequisite: features/arch_design_system.md

## Description
This feature establishes the "LPad Design System" (Theme) for the application. It centralizes color definitions and typography handling, ensuring a consistent visual identity. Crucially, it provides a **runtime architecture** for dynamic theme switching (e.g., Light/Dark mode) without requiring recompilation.

## Architecture

### Theme Definitions
Themes are defined as C++ structures containing color palettes and font pointers.

1.  **`ThemeColors` Struct:** Holds 16-bit RGB565 color values for semantic UI elements:
    - `background`: Primary background for screens.
    - `surface`: Background for cards or elevated elements.
    - `primary`: Main brand color.
    - `secondary`: Secondary brand color.
    - `accent`: Attention-grabbing color.
    - `text_main`: Primary text color.
    - `text_heading`: High-contrast color for section headers (e.g., Cream).
    - `text_secondary`: Subtle text color.
    - `text_highlight`: Highlight color for active selections (e.g., Cream).
    - `text_status`: Network/SSID info (readable).
    - `text_version`: Subtle system metadata.
    - `text_error`: Pure red for critical alerts.
    - `system_menu_background`: Dedicated background for the overlay system menu. This color **must** be a perceptually darker shade of the primary `background` color to ensure the menu is visually distinct.
    - `bg_connecting`: Background color for widgets in a "connecting" state.
    - `scroll_indicator`: Color for the scroll bar indicator.
2.  **`ThemeFonts` Struct:** Holds pointers to `GFXfont` objects for the 5 standardized typography levels.
3.  **`Theme` Struct:** Aggregates `ThemeColors` and `ThemeFonts`.

### Theme Manager
A `ThemeManager` singleton controls the active theme.
-   **`getInstance()`:** Access the singleton.
-   **`getTheme()`:** Returns a pointer to the currently active `Theme` struct.
-   **`setTheme(const Theme* theme)`:** Updates the active theme pointer.
-   **`registerCallback(ThemeChangeCallback cb)`:** (Optional for future) Allows components to react to changes.

### Typography Levels
The system defines 5 standardized typographic levels, exposed via the `ThemeFonts` struct:
1.  **Smallest (9pt):** For ticks, data labels, and dense info.
2.  **Normal (12pt):** For body text and paragraphs.
3.  **UI/Axis (18pt):** For axis labels, button text, and key values.
4.  **Heading (24pt):** For section headers and group titles.
5.  **Title (48pt):** For splash screens and main branding.

## Scenarios

### Scenario: Accessing Active Theme Colors
Given the application is running
When a UI component needs to draw its background
Then it calls `ThemeManager::getInstance().getTheme()->colors.background` to retrieve the correct color for the current state.

### Scenario: Accessing Theme Fonts
Given the application is running
When a UI component needs to render a section header
Then it accesses `ThemeManager::getInstance().getTheme()->fonts.heading` to get the correct `GFXfont` pointer.

### Scenario: Dynamic Theme Switching
Given two theme definitions exist (e.g., `DefaultDark` and `HighContrastLight`)
And the current theme is `DefaultDark`
When the `ThemeManager::getInstance().setTheme(&HighContrastLight)` method is called
Then subsequent calls to `getTheme()` return the properties of the `HighContrastLight` theme
And the UI reflects these changes upon the next redraw.

### Scenario: Default Theme Initialization
Given the application starts up
When the `ThemeManager` is initialized
Then it should load the `Default` theme (located in `src/themes/default`) as the initial active theme.

## Implementation Notes

### [2026-02-16] system_menu_background Field Naming
The `ThemeColors` struct field was renamed from `system_menu_bg` to `system_menu_background` to match the spec. The semantic constant in `theme_colors.h` remains `THEME_SYSTEM_MENU_BG` (short form is acceptable for constants).

### [2026-02-16] system_menu_background Color Correction
Changed from `0x0000` (pure AMOLED black) to `COLOR_DEEP_NIGHT_16` (`0x0881` / `#0C100C`), a 50% darker shade of Night that preserves the green hue. Per spec: "must be a perceptually darker shade of the primary background color." Pure black had no color relationship to Night and created harsh contrast on AMOLED (pixels off vs on).