/**
 * @file ui_widget.cpp
 * @brief GridWidgetLayout and WidgetLayoutEngine implementations
 *
 * Specification: features/ui_widget_framework.md
 */

#include "ui_widget.h"

// ============================================================================
// AnchorPoint → fraction helper
// ============================================================================

void GridWidgetLayout::anchorToFraction(AnchorPoint ap, float& fx, float& fy) {
    switch (ap) {
        case ANCHOR_TOP_LEFT:      fx = 0.0f; fy = 0.0f; break;
        case ANCHOR_TOP_CENTER:    fx = 0.5f; fy = 0.0f; break;
        case ANCHOR_TOP_RIGHT:     fx = 1.0f; fy = 0.0f; break;
        case ANCHOR_LEFT_CENTER:   fx = 0.0f; fy = 0.5f; break;
        case ANCHOR_CENTER:        fx = 0.5f; fy = 0.5f; break;
        case ANCHOR_RIGHT_CENTER:  fx = 1.0f; fy = 0.5f; break;
        case ANCHOR_BOTTOM_LEFT:   fx = 0.0f; fy = 1.0f; break;
        case ANCHOR_BOTTOM_CENTER: fx = 0.5f; fy = 1.0f; break;
        case ANCHOR_BOTTOM_RIGHT:  fx = 1.0f; fy = 1.0f; break;
    }
}

// ============================================================================
// GridWidgetLayout
// ============================================================================

GridWidgetLayout::GridWidgetLayout(int rows, int cols)
    : m_rows(rows), m_cols(cols) {}

void GridWidgetLayout::addWidget(UIWidget* widget, int row, int col,
                                  int rowSpan, int colSpan) {
    if (m_cellCount >= MAX_WIDGETS || widget == nullptr) return;
    m_cells[m_cellCount] = { widget, row, col, rowSpan, colSpan, 0, 0, 0, 0 };
    m_cellCount++;
}

void GridWidgetLayout::calculateLayout(int32_t screenW, int32_t screenH) {
    // 1. Resolve screen reference point to pixel target
    float refFx, refFy;
    anchorToFraction(m_screenRef, refFx, refFy);
    float targetX = refFx * static_cast<float>(screenW) + m_offsetX * static_cast<float>(screenW);
    float targetY = refFy * static_cast<float>(screenH) + m_offsetY * static_cast<float>(screenH);

    // 2. Compute layout pixel dimensions
    m_pixelW = static_cast<int32_t>(m_sizeW * static_cast<float>(screenW));
    m_pixelH = static_cast<int32_t>(m_sizeH * static_cast<float>(screenH));

    // 3. Position layout using anchor point
    float anchorFx, anchorFy;
    anchorToFraction(m_anchor, anchorFx, anchorFy);
    m_pixelX = static_cast<int32_t>(targetX - anchorFx * static_cast<float>(m_pixelW));
    m_pixelY = static_cast<int32_t>(targetY - anchorFy * static_cast<float>(m_pixelH));

    // 4. Compute cell dimensions
    int32_t cellW = (m_cols > 0) ? m_pixelW / m_cols : m_pixelW;
    int32_t cellH = (m_rows > 0) ? m_pixelH / m_rows : m_pixelH;

    // 5. Assign pixel bounding boxes to each widget cell
    for (int i = 0; i < m_cellCount; i++) {
        WidgetCell& cell = m_cells[i];
        cell.pixelX = m_pixelX + cell.col * cellW + cell.widget->paddingX;
        cell.pixelY = m_pixelY + cell.row * cellH + cell.widget->paddingY;
        cell.pixelW = cell.colSpan * cellW - 2 * cell.widget->paddingX;
        cell.pixelH = cell.rowSpan * cellH - 2 * cell.widget->paddingY;

        // Enforce minimums
        if (cell.pixelW < cell.widget->minWidth) cell.pixelW = cell.widget->minWidth;
        if (cell.pixelH < cell.widget->minHeight) cell.pixelH = cell.widget->minHeight;
    }
}

void GridWidgetLayout::render(Arduino_GFX* gfx, int32_t clipMaxY) {
    for (int i = 0; i < m_cellCount; i++) {
        WidgetCell& cell = m_cells[i];
        if (cell.widget == nullptr) continue;

        // Clip: skip widgets entirely below visible area
        if (clipMaxY >= 0 && cell.pixelY >= clipMaxY) continue;

        cell.widget->render(gfx, cell.pixelX, cell.pixelY, cell.pixelW, cell.pixelH);
    }
}

bool GridWidgetLayout::handleInput(const touch_gesture_event_t& event) {
    // Check widgets in reverse order (highest visual priority first)
    for (int i = m_cellCount - 1; i >= 0; i--) {
        WidgetCell& cell = m_cells[i];
        if (cell.widget == nullptr) continue;

        // Hit test: is the touch event within this widget's bounding box?
        if (event.x_px >= cell.pixelX &&
            event.x_px < cell.pixelX + cell.pixelW &&
            event.y_px >= cell.pixelY &&
            event.y_px < cell.pixelY + cell.pixelH) {
            if (cell.widget->handleInput(event, cell.pixelX, cell.pixelY,
                                          cell.pixelW, cell.pixelH)) {
                return true;
            }
        }
    }
    return false;
}

void GridWidgetLayout::update() {
    for (int i = 0; i < m_cellCount; i++) {
        if (m_cells[i].widget) {
            m_cells[i].widget->update();
        }
    }
}

const WidgetCell* GridWidgetLayout::getCell(int index) const {
    if (index >= 0 && index < m_cellCount) return &m_cells[index];
    return nullptr;
}

// ============================================================================
// WidgetLayoutEngine
// ============================================================================

void WidgetLayoutEngine::addLayout(GridWidgetLayout* layout) {
    if (m_layoutCount >= MAX_LAYOUTS || layout == nullptr) return;
    m_layouts[m_layoutCount++] = layout;
}

void WidgetLayoutEngine::calculateLayouts(int32_t screenW, int32_t screenH) {
    for (int i = 0; i < m_layoutCount; i++) {
        m_layouts[i]->calculateLayout(screenW, screenH);
    }
}

void WidgetLayoutEngine::render(Arduino_GFX* gfx, int32_t clipMaxY) {
    for (int i = 0; i < m_layoutCount; i++) {
        m_layouts[i]->render(gfx, clipMaxY);
    }
}

bool WidgetLayoutEngine::handleInput(const touch_gesture_event_t& event) {
    for (int i = m_layoutCount - 1; i >= 0; i--) {
        if (m_layouts[i]->handleInput(event)) return true;
    }
    return false;
}

void WidgetLayoutEngine::update() {
    for (int i = 0; i < m_layoutCount; i++) {
        m_layouts[i]->update();
    }
}
