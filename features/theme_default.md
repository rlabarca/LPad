# Feature: Default Theme

> Label: "Theme: Default"
> Category: "Theme System"
> Prerequisite: features/design_visual_language.md

[TODO]

## 1. Overview

The default theme defines LPad's visual identity using an earth-tone color palette and a 5-level typographic scale. All colors are RGB565 constants organized by semantic role. Fonts are compiled GFX bitmap headers covering JetBrains Mono (system/monospace), Inter (body/general), and Outfit (display/logo) typefaces at five point sizes.

---

## 2. Requirements

### 2.1 Color Palette

- Base colors: Night (#1E231E), Deep Night (#0C100C), Forest (#3F4D39), Sage (#8CA077), Reseda (#6C8E5A), Khaki (#B6AD90), Chamoisee (#A8845A), Cream (#BFB797).
- Semantic mappings MUST be defined for all ThemeColors fields using these base colors.
- Error color: pure red (0xF800). Charging indicator: pure green (0x07E0).

### 2.2 Font Scale

- Level 1 (smallest, 9pt): JetBrains Mono -- ticks, data labels, dense information.
- Level 2 (normal, 12pt): Inter -- body text, paragraphs.
- Level 3 (ui, 18pt): JetBrains Mono -- menu items, axis labels, system text.
- Level 4 (heading, 24pt): Inter -- section headings, group titles.
- Level 5 (title, 48pt): Outfit -- logo, splash screen branding.
- Convenience aliases: `FONT_GRAPH_TICKS` = smallest, `FONT_GRAPH_AXIS_LABELS` = ui, `FONT_GRAPH_DATA_LABELS` = smallest.

### 2.3 Font Compilation

- Font source files (TTF/OTF) are compiled to GFX C headers via `scripts/generate_theme_fonts.sh`.
- Generated headers are stored in `src/themes/default/fonts/` and committed to the repository.
- Font file naming convention: `Font_<Category>_<Size>pt7b.h`.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: All semantic color roles are defined

    Given the default ThemeColors struct
    Then every field (background, surface, primary, secondary, accent, text_main, etc.) has a non-zero RGB565 value

#### Scenario: All font pointers are non-null

    Given the default ThemeFonts struct
    Then smallest, normal, ui, heading, and title are all non-null GFXfont pointers

#### Scenario: Background color is Night

    Given the default theme
    When theme.colors.background is read
    Then it equals 0x1923 (Night)

### Manual Scenarios (Human Verification Required)

#### Scenario: Visual palette coherence on device

    Given the firmware is running on device
    When the stock chart and system menu are viewed
    Then the earth-tone color palette is visually coherent
    And text is legible against all background colors
