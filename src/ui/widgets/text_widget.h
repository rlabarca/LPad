/**
 * @file text_widget.h
 * @brief TextWidget - displays text with wrapping, clipping, and justification
 *
 * Specification: features/ui_standard_widgets.md §1
 */

#ifndef TEXT_WIDGET_H
#define TEXT_WIDGET_H

#include "ui_widget.h"

class Arduino_GFX;

class TextWidget : public UIWidget {
public:
    TextWidget();

    void setText(const char* text) { m_text = text; }
    void setFont(const void* font) { m_font = font; }
    void setColor(uint16_t color) { m_color = color; }
    void setBackgroundColor(uint16_t color) { m_bgColor = color; m_hasBg = true; }

    void render(Arduino_GFX* gfx, int32_t x, int32_t y, int32_t w, int32_t h) override;

private:
    const char* m_text = nullptr;
    const void* m_font = nullptr;
    uint16_t m_color = 0xFFFF;
    uint16_t m_bgColor = 0x0000;
    bool m_hasBg = false;
};

#endif // TEXT_WIDGET_H
