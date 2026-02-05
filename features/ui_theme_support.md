# UI Theme Support

> Label: "Theme Support"
> Category: "UI Framework"
> Prerequisite: features/display_canvas_drawing.md

## Description
This feature establishes the "LPad Design System" (Theme) for the application. It centralizes color definitions and typography handling, ensuring a consistent visual identity across different screens and hardware. The theme support allows for easy switching of color palettes and fonts (e.g., Light/Dark mode) by abstracting specific values behind semantic names.

## Architecture

### Directory Structure
Themes are located in `src/themes/`. The default theme is `src/themes/default/`.
Each theme package contains:
- `theme_colors.h`: Defines color palettes (RGB565 and RGB888).
- `fonts/`: Directory containing generated Adafruit GFX headers.
- `theme_manifest.h`: Aggregates colors and fonts into a usable C++ namespace.

### Typography Levels
The system defines 5 standardized typographic levels:
1.  **Smallest (9pt):** For ticks, data labels, and dense info. (Font: SystemUI/Mono)
2.  **Normal (12pt):** For body text and paragraphs. (Font: General/Inter)
3.  **UI/Axis (18pt):** For axis labels, button text, and key values. (Font: SystemUI/Mono)
4.  **Heading (24pt):** For section headers and group titles. (Font: General/Inter)
5.  **Title (48pt):** For splash screens and main branding. (Font: Logo/Outfit)

## Scenarios

### Scenario: Using Theme Colors
Given the application code needs to draw a background
When the developer uses `LPad::THEME_BACKGROUND`
Then the compiler substitutes the correct 16-bit RGB565 color value (e.g., `0x1923`) defined in the active theme.

### Scenario: Accessing Fonts
Given the application code needs to render a title
When the developer references `LPad::Font_Logo_Title`
Then the application uses the `Font_Logo_48pt7b` GFXfont struct.

### Scenario: Theme Compilation
Given the build system is invoked
Then the `src/themes/default` folder is included in the include path
And the application can `#include "themes/default/theme_manifest.h"` to access the theme assets.
