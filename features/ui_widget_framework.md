# Feature: UI Widget Framework

> Label: "UI Widget Framework"
> Category: "UI Framework"
> Prerequisite: features/arch_ui_widgets.md

## 1. UIWidget Base Class
The abstract base class for all UI elements.

### Attributes
- `minWidth`, `minHeight`: Minimum size in pixels.
- `paddingX`, `paddingY`: Minimum spacing around the widget (default 2px).
- `justificationX`: `LEFT`, `CENTER`, `RIGHT`.
- `justificationY`: `TOP`, `CENTER`, `BOTTOM`.

### Methods
- `virtual void render(int32_t x, int32_t y, int32_t w, int32_t h) = 0`: Draw the widget in the given pixel box.
- `virtual bool handleInput(const touch_gesture_event_t& event, int32_t x, int32_t y, int32_t w, int32_t h)`: Handle input.
- `void setParentLayout(WidgetLayout* layout)`: Association.

## 2. WidgetLayout Base Class
Abstract base for layout heuristics.

### Attributes
- `anchorPoint`: One of 9 anchor points.
- `screenRefPoint`: One of 9 screen reference points.
- `offset`: `float x, y` (relative 0.0-1.0).
- `size`: `float width, height` (relative 0.0-1.0).

### Methods
- `virtual void calculateLayout(int32_t screenW, int32_t screenH) = 0`: 
    - 1. Use `RelativeDisplay` logic to resolve `screenRefPoint`, `offset`, and `size` into a pixel bounding box.
    - 2. Divide that pixel box among child widgets.
- `void addWidget(UIWidget* widget)`: Add a widget to the layout.

## 3. GridWidgetLayout
Arranges widgets in an $M 	imes N$ grid.

### Attributes
- `rows`, `cols`: Number of cells.
- `bounding_box`: Computed pixel area based on relative positioning.

### Logic
- Divides the bounding box into equal cells (or cell spans).
- Places widgets within cells based on their justification.
- Ensures `minWidth`, `minHeight`, and padding are respected.

## 4. WidgetLayoutEngine
Coordinates the rendering and input flow.

### Methods
- `void addLayout(WidgetLayout* layout)`
- `void render()`: Triggers layout calculation and rendering for all layouts.
- `bool handleInput(const touch_gesture_event_t& event)`: Dispatches to the appropriate layout/widget.

## Scenario: Grid Layout Positioning
    Given a GridWidgetLayout with 1 row and 5 columns
    And it is anchored at TOP_CENTER
    And it is 10% down from screen TOP_CENTER
    And its size is 50% width and 50% height
    When `calculateLayout` is called
    Then the bounding box should be centered horizontally
    And its top edge should be at 10% Y (relative)
    And it should be divided into 5 vertical cells.

## Implementation Notes
- **Coordinate System Sync:** Ensure the layout engine uses the same 0.0-1.0 system as `RelativeDisplay`.
- **Anchor Math:** `RelativeDisplay` already has some logic for centering; the layout engine should leverage it or follow the same math (e.g., `y=1.0` is top).
- **GFXfont typedef conflict:** Widget headers must use `const void*` for font pointers, NOT `const GFXfont*`. The C-style `typedef struct { } GFXfont;` in Arduino_GFX conflicts with C++ `struct GFXfont;` forward declarations. Cast to `const GFXfont*` only in .cpp files via `static_cast`.
- **clipMaxY for animation:** GridWidgetLayout::render() accepts a `clipMaxY` parameter to skip rendering cells below the visible area during slide-in/out animation.
