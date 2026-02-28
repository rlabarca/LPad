# Feature: Text Widget

> Label: "Widget: Text"
> Category: "UI Framework"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/design_visual_language.md

[TODO]

## 1. Overview

The TextWidget renders single-line or word-wrapped text within a grid cell. It supports configurable font, color, background fill, underline decoration, and both horizontal and vertical justification. Text that exceeds the cell width is word-wrapped when the cell height permits multiple lines.

---

## 2. Requirements

### 2.1 Configuration

- `setText(text)` sets the display string.
- `setFont(font)` sets the GFXfont pointer.
- `setColor(color)` sets text color (default 0xFFFF white).
- `setBackgroundColor(color)` enables background fill.
- `setUnderlined(bool)` enables a 1px underline below the text.
- Default justification: `JUSTIFY_CENTER_X`, `JUSTIFY_CENTER_Y` (overridden from UIWidget defaults in constructor).

### 2.2 Rendering

- If background is enabled, the entire bounding box is filled first.
- Text bounds are measured via `getTextBounds()`.
- Single-line mode: used when text fits within cell width OR when cell height is too short for two lines. Justification is applied for positioning.
- Word-wrap mode: binary search for maximum chars fitting width, then backtrack to word boundary (space). Each line is rendered with horizontal justification. Vertical advance is `lineHeight = textHeight + 2`.
- Underline: 1px horizontal line at `textY + 2` using the text color.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Short text renders single line centered

    Given a TextWidget with text "Hello" in a 200x50 cell
    When render is called
    Then the text is drawn centered horizontally and vertically

#### Scenario: Long text word-wraps when cell is tall enough

    Given a TextWidget with text exceeding cell width
    And cell height accommodates 3 lines
    When render is called
    Then text is broken at word boundaries across multiple lines

#### Scenario: Background fill covers entire cell

    Given a TextWidget with a background color set
    When render is called
    Then the cell bounding box is filled with the background color before text

#### Scenario: Underline draws below text

    Given a TextWidget with underline enabled
    When render is called
    Then a 1px horizontal line appears 2px below the text baseline

### Manual Scenarios (Human Verification Required)

None.
