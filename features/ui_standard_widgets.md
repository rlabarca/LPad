# Feature: Standard UI Widgets

> Label: "Standard UI Widgets"
> Category: "UI Framework"
> Prerequisite: features/ui_widget_framework.md
> Prerequisite: features/arch_design_system.md

## 1. TextWidget
A simple widget for displaying text.

### Capabilities
- **Wrapping:** Text wraps to next line if it exceeds width.
- **Clipping:** Text is clipped if it exceeds the height of the bounding box.
- **Justification:** Supports X (Left, Center, Right) and Y (Top, Center, Bottom) alignment.
- **Styling:**
    - Configurable font (via theme), size, color, and background color.
    - **isUnderlined:** If true, a 1px line is drawn 2 pixels below the text baseline, matching the text color.

## 2. ScrollableListWidget
A list of text items that can be scrolled.

### Capabilities
- **Single-Line Items:** Each item is one line; horizontal overflow is clipped.
- **Scrolling:** Vertical scroll via tap-and-drag.
- **Scroll Indicator:** 2px wide vertical bar on the right side if content exceeds height.
- **Selection:** Responds to taps, returning the index of the selected item.
- **Status Indicators (Circles):**
    - Supports a small, filled circle drawn next to any list item.
    - **Uniform Position:** The circle is positioned either on the LEFT or RIGHT of the text for the entire list (cannot vary per item).
    - **Configurable Color:** The circle color can be set per item.
    - **Alignment:** 
        - If circles are on the LEFT, ALL text entries must be shifted right (indented) so they remain aligned, regardless of whether a specific item currently has a circle drawn.
        - If circles are on the RIGHT, text must be clipped to ensure it does not overlap the circle area.
- **Styling:**
    - Items can have different font colors.
    - Uniform font for the whole list.
    - Configurable vertical padding between items.
    - Background color for the list.

## Scenario: Text Wrapping
    Given a TextWidget with text "This is a long string that should wrap"
    And a bounding box that can only fit 10 characters per line
    When `render()` is called
    Then the text should be split into multiple lines
    And subsequent lines should only be drawn if they fit within the height.

## Scenario: List Scrolling
    Given a ScrollableListWidget with 20 items
    And a bounding box that can only show 5 items
    When the User drags UP on the widget
    Then the items should move UP
    And the scroll indicator should move DOWN
    And different items should become visible.

## Implementation Notes
- **Text Height:** Use `gfx->getTextBounds` or equivalent to calculate the height of a line for proper wrapping and vertical justification.
- **Scroll Physics:** Simple linear mapping of drag distance to scroll offset. No momentum needed for v1.
- **Hit Testing:** When a tap occurs, calculate the index: `index = (tap_y - start_y + scroll_offset) / line_height`.
- **Font type erasure:** Both TextWidget and ScrollableListWidget store fonts as `const void*` to avoid GFXfont typedef conflicts across compilation units. See ui_widget_framework.md notes.
- **Line height computation:** ScrollableListWidget computes lineHeight from font using `getTextBounds("Ay", ...)` on first render. Falls back to 20px when no font is set (useful in native tests where mock GFX returns 0).
- **Circle implementation:** `CirclePosition` enum (NONE/LEFT/RIGHT) controls uniform position. `CIRCLE_INDENT` (12px) reserves space when LEFT is active — ALL items are indented regardless of whether they have a circle, ensuring alignment. Circle is drawn as `fillCircle` with `CIRCLE_RADIUS` (3px), vertically centered in the line.
- **TextWidget single-line fallback:** If the cell height is too small for word-wrap (< 2× lineHeight), the text renders as a single centered line regardless of width. This prevents the word-wrap path from rendering nothing when a large font (e.g., 24pt heading, yAdvance=57) is placed in a compact grid cell (e.g., 44px). GFX overflow-clips naturally.
