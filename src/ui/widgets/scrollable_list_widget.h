/**
 * @file scrollable_list_widget.h
 * @brief ScrollableListWidget - scrollable list of text items with selection
 *
 * Specification: features/ui_standard_widgets.md §2
 */

#ifndef SCROLLABLE_LIST_WIDGET_H
#define SCROLLABLE_LIST_WIDGET_H

#include "ui_widget.h"

class Arduino_GFX;

class ScrollableListWidget : public UIWidget {
public:
    static constexpr int MAX_ITEMS = 32;

    struct ListItem {
        const char* text = nullptr;
        uint16_t textColor = 0xFFFF;
        uint16_t bgColor = 0x0000;
        bool hasBg = false;
    };

    ScrollableListWidget();

    void setFont(const void* font) { m_font = font; }
    void setBackgroundColor(uint16_t color) { m_bgColor = color; }
    void setItemPadding(int32_t padding) { m_itemPadding = padding; }
    void setScrollIndicatorColor(uint16_t color) { m_scrollIndicatorColor = color; }

    int addItem(const char* text, uint16_t color = 0xFFFF);
    void setItemColor(int index, uint16_t color);
    void setItemBackground(int index, uint16_t color);
    void clearItemBackground(int index);
    void clearItems();

    int getItemCount() const { return m_itemCount; }
    int getSelectedIndex() const { return m_selectedIndex; }
    int getScrollOffset() const { return m_scrollOffset; }

    void render(Arduino_GFX* gfx, int32_t x, int32_t y, int32_t w, int32_t h) override;
    bool handleInput(const touch_gesture_event_t& event,
                     int32_t x, int32_t y, int32_t w, int32_t h) override;

    /** Callback fired when an item is tapped. */
    using SelectionCallback = void(*)(int index, void* context);
    void setSelectionCallback(SelectionCallback cb, void* ctx = nullptr) {
        m_callback = cb;
        m_callbackCtx = ctx;
    }

protected:
    ListItem m_items[MAX_ITEMS];
    int m_itemCount = 0;
    int m_selectedIndex = -1;
    int m_scrollOffset = 0;

    const void* m_font = nullptr;
    uint16_t m_bgColor = 0x0000;
    int32_t m_itemPadding = 4;
    uint16_t m_scrollIndicatorColor = 0x7BEF;
    int32_t m_lineHeight = 20;  // Computed from font, fallback 20px

    SelectionCallback m_callback = nullptr;
    void* m_callbackCtx = nullptr;

    int getVisibleItemCount(int32_t h) const;
    int getItemAtY(int32_t tapY, int32_t boxY, int32_t boxH) const;
};

#endif // SCROLLABLE_LIST_WIDGET_H
