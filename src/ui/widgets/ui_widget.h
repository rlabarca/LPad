/**
 * @file ui_widget.h
 * @brief UI Widget Framework - Base classes, GridWidgetLayout, WidgetLayoutEngine
 *
 * Provides a structured way to compose UI elements using relative positioning,
 * layout heuristics, and uniform event handling. Sits on top of RelativeDisplay
 * and UIRenderManager.
 *
 * Specification: features/ui_widget_framework.md
 * Architecture:  features/arch_ui_widgets.md
 */

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <stdint.h>
#include "../../input/touch_gesture_engine.h"

class Arduino_GFX;

// ============================================================================
// Anchor / Reference Points (9-point system)
// ============================================================================

enum AnchorPoint {
    ANCHOR_TOP_LEFT,
    ANCHOR_TOP_CENTER,
    ANCHOR_TOP_RIGHT,
    ANCHOR_LEFT_CENTER,
    ANCHOR_CENTER,
    ANCHOR_RIGHT_CENTER,
    ANCHOR_BOTTOM_LEFT,
    ANCHOR_BOTTOM_CENTER,
    ANCHOR_BOTTOM_RIGHT
};

// ============================================================================
// Justification enums
// ============================================================================

enum JustificationX { JUSTIFY_LEFT, JUSTIFY_CENTER_X, JUSTIFY_RIGHT };
enum JustificationY { JUSTIFY_TOP, JUSTIFY_CENTER_Y, JUSTIFY_BOTTOM };

// ============================================================================
// UIWidget - Abstract base class for all UI widgets
// ============================================================================

class UIWidget {
public:
    virtual ~UIWidget() = default;

    /**
     * Render the widget within the given pixel bounding box.
     * @param gfx  Target GFX canvas
     * @param x,y  Top-left corner (pixels)
     * @param w,h  Dimensions (pixels)
     */
    virtual void render(Arduino_GFX* gfx, int32_t x, int32_t y, int32_t w, int32_t h) = 0;

    /**
     * Handle a touch event. Coordinates in the event are screen-absolute.
     * The bounding box is provided so the widget can do relative calculations.
     * @return true if the event was consumed.
     */
    virtual bool handleInput(const touch_gesture_event_t& event,
                             int32_t x, int32_t y, int32_t w, int32_t h) {
        (void)event; (void)x; (void)y; (void)w; (void)h;
        return false;
    }

    /** Called periodically for state updates (e.g., polling network status). */
    virtual void update() {}

    // Size constraints (pixels)
    int32_t minWidth = 0;
    int32_t minHeight = 0;

    // Padding (pixels, default 2px per spec)
    int32_t paddingX = 2;
    int32_t paddingY = 2;

    // Content justification within assigned cell
    JustificationX justificationX = JUSTIFY_LEFT;
    JustificationY justificationY = JUSTIFY_TOP;
};

// ============================================================================
// WidgetCell - Widget placement within a grid
// ============================================================================

struct WidgetCell {
    UIWidget* widget = nullptr;
    int row = 0;
    int col = 0;
    int rowSpan = 1;
    int colSpan = 1;

    // Computed pixel bounding box (set during calculateLayout)
    int32_t pixelX = 0;
    int32_t pixelY = 0;
    int32_t pixelW = 0;
    int32_t pixelH = 0;
};

// ============================================================================
// GridWidgetLayout - Arranges widgets in an M x N grid
// ============================================================================

class GridWidgetLayout {
public:
    static constexpr int MAX_WIDGETS = 16;

    GridWidgetLayout(int rows, int cols);

    // Positioning (0.0-1.0 relative coordinates)
    void setAnchorPoint(AnchorPoint anchor) { m_anchor = anchor; }
    void setScreenRefPoint(AnchorPoint ref) { m_screenRef = ref; }
    void setOffset(float x, float y) { m_offsetX = x; m_offsetY = y; }
    void setSize(float w, float h) { m_sizeW = w; m_sizeH = h; }

    /** Add a widget to the grid at the specified cell position with optional spanning. */
    void addWidget(UIWidget* widget, int row, int col, int rowSpan = 1, int colSpan = 1);

    /** Resolve relative positioning to pixel bounding boxes. */
    void calculateLayout(int32_t screenW, int32_t screenH);

    /** Render all widgets (optionally clipped to clipMaxY). */
    void render(Arduino_GFX* gfx, int32_t clipMaxY = -1);

    /** Route touch events to the appropriate widget via hit testing. */
    bool handleInput(const touch_gesture_event_t& event);

    /** Call update() on all child widgets. */
    void update();

    // Accessors for testing
    int32_t getPixelX() const { return m_pixelX; }
    int32_t getPixelY() const { return m_pixelY; }
    int32_t getPixelW() const { return m_pixelW; }
    int32_t getPixelH() const { return m_pixelH; }
    int getRows() const { return m_rows; }
    int getCols() const { return m_cols; }
    const WidgetCell* getCell(int index) const;
    int getCellCount() const { return m_cellCount; }

private:
    int m_rows;
    int m_cols;

    AnchorPoint m_anchor = ANCHOR_TOP_LEFT;
    AnchorPoint m_screenRef = ANCHOR_TOP_LEFT;
    float m_offsetX = 0.0f;
    float m_offsetY = 0.0f;
    float m_sizeW = 1.0f;
    float m_sizeH = 1.0f;

    // Computed pixel bounding box
    int32_t m_pixelX = 0;
    int32_t m_pixelY = 0;
    int32_t m_pixelW = 0;
    int32_t m_pixelH = 0;

    WidgetCell m_cells[MAX_WIDGETS];
    int m_cellCount = 0;

    static void anchorToFraction(AnchorPoint ap, float& fx, float& fy);
};

// ============================================================================
// WidgetLayoutEngine - Coordinates multiple layouts
// ============================================================================

class WidgetLayoutEngine {
public:
    static constexpr int MAX_LAYOUTS = 4;

    void addLayout(GridWidgetLayout* layout);
    void calculateLayouts(int32_t screenW, int32_t screenH);
    void render(Arduino_GFX* gfx, int32_t clipMaxY = -1);
    bool handleInput(const touch_gesture_event_t& event);
    void update();

    int getLayoutCount() const { return m_layoutCount; }

private:
    GridWidgetLayout* m_layouts[MAX_LAYOUTS] = {};
    int m_layoutCount = 0;
};

#endif // UI_WIDGET_H
