# Feature: UI Widget Framework

> Label: "UI: Widget Framework"
> Category: "UI Framework"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/arch_display_pipeline.md

[TODO]

## 1. Overview

The widget framework provides a grid-based layout system for composing UI elements within components. `UIWidget` is the abstract base class for all widgets. `GridWidgetLayout` arranges widgets in a row/column grid with anchor-point positioning and relative sizing. `WidgetLayoutEngine` aggregates multiple grid layouts for complex screens. The framework supports 9-point anchor positioning, fractional screen offsets, and automatic cell sizing.

---

## 2. Requirements

### 2.1 UIWidget Base

- MUST define pure virtual `render(gfx, x, y, w, h)` receiving the computed pixel bounding box.
- `handleInput(event, x, y, w, h)` MUST default to returning `false`.
- `update()` MUST default to no-op.
- Default padding: `paddingX=2`, `paddingY=2` pixels.
- Default justification: `JUSTIFY_LEFT`, `JUSTIFY_TOP`.
- `minWidth` and `minHeight` default to 0.

### 2.2 GridWidgetLayout

- MUST support up to 16 widgets in a rows x columns grid.
- `addWidget(widget, row, col, rowSpan, colSpan)` MUST place a widget spanning the specified cells.
- `setAnchorPoint(anchor)` MUST define the layout's pivot point from 9-point AnchorPoint enum.
- `setScreenRefPoint(ref)` MUST define the screen position the anchor attaches to.
- `setOffset(x, y)` MUST specify fractional (0.0-1.0) screen offsets from the reference point.
- `setSize(w, h)` MUST specify fractional screen dimensions.
- `calculateLayout(screenW, screenH)` MUST resolve anchor math to pixel coordinates and divide cells equally across rows and columns, subtracting widget padding and enforcing minimum sizes.
- `render(gfx, clipMaxY)` MUST skip cells whose `pixelY >= clipMaxY` when clipMaxY >= 0.
- `handleInput(event)` MUST hit-test cells in reverse order (last added = highest priority) using `event.x_px/y_px` against pixel bounding boxes.

### 2.3 WidgetLayoutEngine

- MUST support up to 4 GridWidgetLayouts.
- `render()` MUST call all layouts in order.
- `handleInput()` MUST call layouts in reverse order, stopping at the first consumer.
- `update()` MUST call all layouts.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Grid layout calculates equal cell sizes

    Given a GridWidgetLayout with 2 rows and 2 columns
    And size is set to (0.5, 0.5) of a 400x400 screen
    When calculateLayout is called
    Then each cell is 100x100 pixels minus padding

#### Scenario: Anchor point offsets layout position

    Given a GridWidgetLayout with anchor ANCHOR_CENTER
    And screen reference ANCHOR_CENTER at offset (0,0) on a 400x400 screen
    When calculateLayout is called
    Then the layout is centered on screen

#### Scenario: Widget minimum size is enforced

    Given a widget with minWidth=50 in a cell that computes to 30px
    When calculateLayout is called
    Then the cell width is 50px

#### Scenario: Clip max Y hides lower cells

    Given a layout with cells at y=100 and y=200
    When render is called with clipMaxY=150
    Then only the cell at y=100 is rendered

#### Scenario: Input hit-tests cells in reverse order

    Given two overlapping widgets at different grid positions
    When handleInput is called with a tap inside the overlap region
    Then the later-added widget receives the event first

#### Scenario: Layout engine stops at first input consumer

    Given a WidgetLayoutEngine with two layouts
    And the second layout's widget returns true from handleInput
    When handleInput is called
    Then the first layout does not receive the event

### Manual Scenarios (Human Verification Required)

None.
