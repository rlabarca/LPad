# Architectural Policy: UI Widget System

> Label: "UI Widget Policy"
> Category: "ARCHITECTURES"
> Prerequisite: features/arch_ui_compositing.md
> Prerequisite: features/display_relative_drawing.md

## 1. Goal
The UI Widget System provides a structured way to compose UI elements using relative positioning, layout heuristics, and uniform event handling. It sits on top of the `RelativeDisplay` and `UIRenderManager`.

## 2. Core Concepts

### 2.1 The Widget (`UIWidget`)
A `UIWidget` is a self-contained UI element that knows how to:
1.  **Measure itself:** Provide minimum dimensions and padding in pixels.
2.  **Render itself:** Draw within a provided bounding box (pixels).
3.  **Handle Input:** Respond to touch events within its bounding box.

### 2.2 Layout Heuristics (`WidgetLayout`)
A `WidgetLayout` is the bridge between the relative and pixel coordinate systems.
*   **External Positioning:** Layouts use **Relative Coordinates** (0.0 - 1.0 float, where 1.0 = 100% of screen) to define their own position and size.
*   **The Pixel Hand-off:** The Layout translates its relative bounding box into concrete **integer pixel coordinates** (x, y, width, height).
*   **Internal Distribution:** The Layout calculates how to distribute its assigned pixels to its children, respecting the child's **pixel-based** `minWidth`, `minHeight`, and `padding`.

### 2.3 The Engine (`WidgetLayoutEngine`)
The `WidgetLayoutEngine` manages the relationship between widgets and layouts. It queries `RelativeDisplay` to resolve relative floats into pixels before calling the Layout's calculation logic.

## 3. Positioning & Anchoring

### 3.1 Anchor Points
Both Layouts and Widgets (within a layout) can use 9 anchor points:
*   `TOP_LEFT`, `TOP_CENTER`, `TOP_RIGHT`
*   `LEFT_CENTER`, `CENTER`, `RIGHT_CENTER`
*   `BOTTOM_LEFT`, `BOTTOM_CENTER`, `BOTTOM_RIGHT`

### 3.2 Layout Anchoring Logic
A layout is positioned by:
1.  Defining its **Anchor Point** (e.g., `TOP_RIGHT`).
2.  Defining a **Screen Reference Point** (e.g., `RIGHT_CENTER`).
3.  Defining an **Offset** in relative coordinates (e.g., `x=0.1`, `y=0.0` which is 10% width, 0% height).
4.  The Layout's `TOP_RIGHT` will be placed at (Screen `RIGHT_CENTER` + Offset).
5.  All float math is resolved to pixels via `RelativeDisplay` before rendering.

## 4. Interaction Model
*   The parent component (e.g., `SystemMenuComponent`) receives touch events from `UIRenderManager`.
*   It passes these events to the `WidgetLayoutEngine`.
*   The Engine/Layout determines which widget intersects the touch and forwards the event.
*   Widgets return `true` if they consumed the event.

## 5. Invariants
*   Widgets **must not** draw outside their assigned bounding box.
*   Layouts **must** respect the `minWidth` and `minHeight` of widgets, or clip/scale them if space is insufficient (policy-dependent).
*   Relative coordinates are used for high-level placement; pixel coordinates are used for internal widget rendering and padding.
