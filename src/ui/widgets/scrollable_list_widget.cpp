/**
 * @file scrollable_list_widget.cpp
 * @brief ScrollableListWidget implementation
 *
 * Specification: features/ui_standard_widgets.md §2
 * Implementation Notes:
 *   - Line height computed from font via getTextBounds("Ay", ...)
 *   - Scroll physics: simple linear offset, no momentum (v1)
 *   - Hit testing: index = (tap_y - start_y + scroll_offset) / line_height
 */

#include "scrollable_list_widget.h"
#include <Arduino_GFX_Library.h>

ScrollableListWidget::ScrollableListWidget() {
    paddingX = 4;
    paddingY = 4;
}

int ScrollableListWidget::addItem(const char* text, uint16_t color) {
    if (m_itemCount >= MAX_ITEMS) return -1;
    m_items[m_itemCount].text = text;
    m_items[m_itemCount].textColor = color;
    m_items[m_itemCount].bgColor = 0x0000;
    m_items[m_itemCount].circleColor = 0xFFFF;
    m_items[m_itemCount].hasBg = false;
    m_items[m_itemCount].hasCircle = false;
    return m_itemCount++;
}

void ScrollableListWidget::setItemColor(int index, uint16_t color) {
    if (index >= 0 && index < m_itemCount) {
        m_items[index].textColor = color;
    }
}

void ScrollableListWidget::setItemBackground(int index, uint16_t color) {
    if (index >= 0 && index < m_itemCount) {
        m_items[index].bgColor = color;
        m_items[index].hasBg = true;
    }
}

void ScrollableListWidget::clearItemBackground(int index) {
    if (index >= 0 && index < m_itemCount) {
        m_items[index].hasBg = false;
    }
}

void ScrollableListWidget::setItemCircle(int index, uint16_t color) {
    if (index >= 0 && index < m_itemCount) {
        m_items[index].circleColor = color;
        m_items[index].hasCircle = true;
    }
}

void ScrollableListWidget::clearItemCircle(int index) {
    if (index >= 0 && index < m_itemCount) {
        m_items[index].hasCircle = false;
    }
}

void ScrollableListWidget::clearItems() {
    m_itemCount = 0;
    m_scrollOffset = 0;
    m_selectedIndex = -1;
}

int ScrollableListWidget::getVisibleItemCount(int32_t h) const {
    if (m_lineHeight <= 0) return 0;
    return static_cast<int>(h / m_lineHeight);
}

int ScrollableListWidget::getItemAtY(int32_t tapY, int32_t boxY, int32_t boxH) const {
    if (m_lineHeight <= 0) return -1;
    int32_t relY = tapY - boxY;
    if (relY < 0 || relY >= boxH) return -1;
    int index = m_scrollOffset + static_cast<int>(relY / m_lineHeight);
    if (index >= m_itemCount) return -1;
    return index;
}

void ScrollableListWidget::render(Arduino_GFX* gfx, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (gfx == nullptr) return;

    // Background fill
    gfx->fillRect(x, y, w, h, m_bgColor);

    // Compute line height from font
    if (m_font != nullptr) {
        gfx->setFont(static_cast<const GFXfont*>(m_font));
        int16_t x1, y1;
        uint16_t tw, th;
        gfx->getTextBounds("Ay", 0, 0, &x1, &y1, &tw, &th);
        if (th > 0) {
            m_lineHeight = static_cast<int32_t>(th) + m_itemPadding * 2;
        }
    }

    int visible = getVisibleItemCount(h);
    int32_t itemY = y;

    // Text indent when circles on LEFT (spec: all items shift right for alignment)
    int32_t textIndent = 0;
    if (m_circlePosition == CIRCLE_LEFT) {
        textIndent = CIRCLE_INDENT;
    }

    for (int i = m_scrollOffset; i < m_itemCount && i < m_scrollOffset + visible; i++) {
        const ListItem& item = m_items[i];

        // Item background (leave 3px right margin for scroll indicator)
        if (item.hasBg) {
            gfx->fillRect(x, itemY, w - 3, m_lineHeight, item.bgColor);
        }

        // Status circle (LEFT or RIGHT of text)
        if (item.hasCircle && m_circlePosition != CIRCLE_NONE) {
            int32_t cy = itemY + m_lineHeight / 2;
            int32_t cx;
            if (m_circlePosition == CIRCLE_LEFT) {
                cx = x + paddingX + CIRCLE_RADIUS;
            } else {
                cx = x + w - 3 - paddingX - CIRCLE_RADIUS;
            }
            gfx->fillCircle(cx, cy, CIRCLE_RADIUS, item.circleColor);
        }

        // Item text
        if (item.text != nullptr) {
            gfx->setFont(static_cast<const GFXfont*>(m_font));
            gfx->setTextColor(item.textColor);
            // Position: left-padded (+ indent if circles on LEFT)
            gfx->setCursor(x + paddingX + textIndent, itemY + m_lineHeight - m_itemPadding);
            gfx->print(item.text);
        }

        itemY += m_lineHeight;
    }

    // 2px scroll indicator on the right edge
    if (m_itemCount > visible && visible > 0) {
        int32_t indicatorH = (h * visible) / m_itemCount;
        if (indicatorH < 8) indicatorH = 8;

        int32_t maxScroll = m_itemCount - visible;
        int32_t indicatorY = y;
        if (maxScroll > 0) {
            indicatorY = y + (m_scrollOffset * (h - indicatorH)) / maxScroll;
        }

        gfx->fillRect(x + w - 2, indicatorY, 2, indicatorH, m_scrollIndicatorColor);
    }
}

bool ScrollableListWidget::handleInput(const touch_gesture_event_t& event,
                                        int32_t x, int32_t y, int32_t w, int32_t h) {
    (void)w;

    // TAP → select item
    if (event.type == TOUCH_TAP) {
        int index = getItemAtY(event.y_px, y, h);
        if (index >= 0) {
            m_selectedIndex = index;
            if (m_callback) {
                m_callback(index, m_callbackCtx);
            }
            return true;
        }
    }

    // SWIPE → scroll list
    if (event.type == TOUCH_SWIPE) {
        int visible = getVisibleItemCount(h);
        int scrollAmount = (visible > 2) ? visible / 2 : 1;
        int maxScroll = m_itemCount - visible;
        if (maxScroll < 0) maxScroll = 0;

        if (event.direction == TOUCH_DIR_UP) {
            // Finger moves up → show items further down
            m_scrollOffset += scrollAmount;
            if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
            return true;
        } else if (event.direction == TOUCH_DIR_DOWN) {
            // Finger moves down → show items further up
            m_scrollOffset -= scrollAmount;
            if (m_scrollOffset < 0) m_scrollOffset = 0;
            return true;
        }
    }

    return false;
}
