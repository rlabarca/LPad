# Architectural Policy: Design System

> Label: "Design System Policy"
> Category: "ARCHITECTURES"
> Prerequisite: None

## 1. Theme Architecture
*   **Isolation:** Themes are stored in `src/themes/<theme_name>/`.
*   **Manifest Pattern:** Every theme MUST provide a `theme_manifest.h` which acts as the entry point.
*   **Semantic Mapping:** Code **MUST NOT** use hardcoded hex colors. It must use semantic names defined in the `ThemeColors` structure:
    *   `background`, `surface`, `primary`, `secondary`, `accent` (Core Palette).
    *   `text_main`, `text_secondary` (Standard Typography).
    *   `text_error` (Critical alerts/errors - formerly `0xF800`).
    *   `text_version` (System metadata - formerly `0x7BEF`).
    *   `text_status` (Network/SSID info - formerly `0xFFFF`).
    *   `system_menu_bg` (System Menu overlay background).
*   **Default Fallbacks:** Default themes MUST provide high-contrast values for all semantic keys.

## 2. Typography & Fonts
*   **GFX Fonts:** We use Adafruit GFX compatible header-based fonts.
*   **Standard Levels:**
    *   `FONT_SMALLEST` (9pt Mono)
    *   `FONT_NORMAL` (12pt Sans)
    *   **UI/Axis** (18pt Mono)
    *   **Heading** (24pt Sans)
    *   **Title** (48pt Branding)
*   **Font Conversion:** Managed by `scripts/generate_theme_fonts.sh`.

## 3. Asset Management (Vector-First)
To ensure resolution independence and maximize memory efficiency on memory-constrained hardware (ESP32-S3), the system prioritizes **Vector Assets** (SVG).
*   **Memory Efficiency:** Vector meshes occupy significantly less Flash and RAM than pre-rendered bitmaps.
*   **Resolution Independence:** Vector assets scale perfectly to any display size.
*   **Asset Pipeline:** `scripts/process_svgs.py` compiles `.svg` files into C++ vertex meshes stored in Flash memory.
