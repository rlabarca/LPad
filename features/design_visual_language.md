# Design: Visual Language

> Label: "Design: Visual Language"
> Category: "Design"

## Purpose

Defines the visual design system for LPad firmware. All UI rendering is theme-driven through the `ThemeManager` singleton, which provides a centralized color palette (RGB565), a 5-level semantic font scale, and gradient definitions. The relative coordinate system (0-100%) ensures visual layouts are resolution-independent across hardware targets.

## Visual Language Invariants

### Color System

- All colors MUST be defined in the active theme's `ThemeColors` struct as named RGB565 constants.
- Semantic color roles (background, text, accent, graph line, etc.) MUST be accessed via `ThemeManager::getTheme()->colors`, never as hardcoded color values in application code.
- The default palette uses an earth-tone scheme: Night (dark background), Forest, Sage, Khaki, Chamoisee, Cream.

### Typography

- The font system uses a 5-level semantic scale mapped in `ThemeFonts`:
  - Level 1 (Smallest): 9pt monospace (JetBrains Mono) -- data labels, timestamps
  - Level 2 (Normal): 12pt sans-serif (Inter/General) -- body text
  - Level 3 (UI): 18pt monospace (JetBrains Mono) -- menu items, system text
  - Level 4 (Heading): 24pt sans-serif (Inter/General) -- section headings
  - Level 5 (Title): 48pt display (Outfit/Logo) -- logo, splash screen
- Application code MUST reference fonts by semantic level via the theme, never by direct font object name.

### Coordinate System

- All drawing positions and dimensions are expressed as percentages (0.0-100.0) of screen dimensions via `RelativeDisplay`.
- This makes layouts hardware-agnostic across portrait (368x448) and landscape (448x368) orientations.

### Gradient Support

- The system supports `LinearGradient` and `RadialGradient` structs for background fills and indicator effects.
- Gradients are rendered using the theme's color palette entries.

## Scenarios

No automated or manual scenarios. This is a policy anchor node -- its "scenarios" are
process invariants enforced by instruction files and tooling.
