# Feature: Theme Manager

> Label: "Theme: Manager"
> Category: "Theme System"
> Prerequisite: features/design_visual_language.md

[TODO]

## 1. Overview

The ThemeManager is a singleton that provides centralized access to the active theme throughout the LPad firmware. A Theme aggregates ThemeColors (RGB565 palette with semantic roles) and ThemeFonts (5-level font scale). The manager supports runtime theme switching, though currently only a single default theme is implemented.

---

## 2. Requirements

### 2.1 Singleton Access

- `ThemeManager::getInstance()` MUST return the same instance on every call (Meyer's singleton).
- The default theme is set as the active theme at construction.

### 2.2 Theme Access

- `getTheme()` MUST return a pointer to the currently active Theme.
- `getDefaultTheme()` MUST return a static pointer to the built-in default theme.

### 2.3 Theme Switching

- `setTheme(theme)` MUST replace the active theme pointer. Passing null MUST be a no-op.

### 2.4 Theme Structure

- `Theme` aggregates `ThemeColors` and `ThemeFonts`.
- `ThemeColors` MUST define semantic color roles: background, surface, primary, secondary, accent, text_main, text_secondary, text_error, text_version, text_status, graph_axes, graph_ticks, axis_labels, data_labels, system_menu_background, text_heading, text_highlight, bg_connecting, scroll_indicator.
- `ThemeFonts` MUST define 5 semantic font levels: smallest (9pt), normal (12pt), ui (18pt), heading (24pt), title (48pt).

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Singleton returns same instance

    Given ThemeManager::getInstance is called twice
    Then both calls return the same reference

#### Scenario: Default theme is active at startup

    Given the ThemeManager is freshly constructed
    When getTheme is called
    Then it returns the default theme

#### Scenario: setTheme with null is a no-op

    Given a theme is currently active
    When setTheme(nullptr) is called
    Then getTheme still returns the previous theme

#### Scenario: Theme switch replaces active theme

    Given the default theme is active
    When setTheme is called with a custom theme
    Then getTheme returns the custom theme

### Manual Scenarios (Human Verification Required)

None.
