# Feature: Scrollable List Widget

> Label: "Widget: Scrollable List"
> Category: "UI Framework"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/design_visual_language.md

[TODO]

## 1. Overview

The ScrollableListWidget provides a vertically scrollable text list with tap selection, colored circle indicators, per-item background colors, and a proportional scroll indicator bar. It supports up to 32 items and handles TAP (selection), SWIPE UP (scroll down), and SWIPE DOWN (scroll up) gestures.

---

## 2. Requirements

### 2.1 Item Management

- MUST support up to 32 list items (MAX_ITEMS).
- `addItem(text, color)` MUST return the item index or -1 if full.
- Each item MAY have: custom text color, background color, and a colored circle indicator.
- `clearItems()` MUST reset count, scroll offset, and selection.

### 2.2 Rendering

- Items are rendered within the bounding box provided by the parent GridWidgetLayout.
- Line height is computed from font metrics (`getTextBounds("Ay")`) plus item padding.
- Only visible items (from `scrollOffset` to `scrollOffset + visibleCount`) are rendered.
- If an item has a background, it fills the item row (with 3px right margin for scroll indicator).
- Circle indicators (CIRCLE_LEFT or CIRCLE_RIGHT) are drawn at the vertical center of the row.
- Text is indented by 12px (CIRCLE_INDENT) when circle position is CIRCLE_LEFT.

### 2.3 Scroll Indicator

- A 2px-wide proportional scroll bar is drawn at the right edge.
- Bar height = `(visibleCount / totalCount) * listHeight`, minimum 8px.
- Bar position is proportional to the scroll offset.

### 2.4 Input Handling

- TAP: hit-tests using `(tapY - boxY) / lineHeight + scrollOffset` to determine the selected item. Fires the selection callback.
- SWIPE UP: scrolls down by `visibleCount / 2` (minimum 1). Clamps to `itemCount - visibleCount`.
- SWIPE DOWN: scrolls up by `visibleCount / 2`. Clamps to 0.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Add item returns sequential indices

    Given an empty ScrollableListWidget
    When addItem is called 3 times
    Then indices 0, 1, 2 are returned

#### Scenario: Add item rejects when full

    Given 32 items are already added
    When addItem is called
    Then it returns -1

#### Scenario: Tap selects correct item accounting for scroll offset

    Given 10 items with scroll offset 3
    And visible count is 5
    When a TAP is received at the 2nd visible row
    Then the selection callback fires with index 5

#### Scenario: Swipe up scrolls down by half page

    Given 20 items with visible count 8 and scroll offset 0
    When a SWIPE UP gesture is received
    Then scroll offset becomes 4

#### Scenario: Scroll clamps to bottom

    Given 10 items with visible count 5 and scroll offset 4
    When a SWIPE UP gesture is received
    Then scroll offset remains at 5 (10 - 5)

#### Scenario: Clear items resets all state

    Given items and a scroll offset exist
    When clearItems is called
    Then item count is 0 and scroll offset is 0 and selected index is -1

### Manual Scenarios (Human Verification Required)

None.
