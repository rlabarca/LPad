/**
 * @file text_widget.cpp
 * @brief TextWidget implementation with word-wrap and justification
 *
 * Specification: features/ui_standard_widgets.md §1
 * Implementation Note: Uses gfx->getTextBounds for line height calculation.
 */

#include "text_widget.h"
#include <Arduino_GFX_Library.h>
#include <string.h>

TextWidget::TextWidget() {
    justificationX = JUSTIFY_CENTER_X;
    justificationY = JUSTIFY_CENTER_Y;
}

void TextWidget::render(Arduino_GFX* gfx, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (gfx == nullptr || m_text == nullptr || m_text[0] == '\0') return;

    // Fill background if configured
    if (m_hasBg) {
        gfx->fillRect(x, y, w, h, m_bgColor);
    }

    gfx->setFont(static_cast<const GFXfont*>(m_font));
    gfx->setTextColor(m_color);

    // Measure full text
    int16_t x1, y1;
    uint16_t tw, th;
    gfx->getTextBounds(m_text, 0, 0, &x1, &y1, &tw, &th);

    // Single-line rendering: text fits width, OR cell too short for word-wrap.
    // When the cell height can't hold even one wrapped line, render the full text
    // as a single centered line (GFX will overflow-clip naturally).
    int32_t lineHeight = (th > 0) ? (int32_t)th + 2 : 14;
    bool fitsWidth = ((int32_t)tw <= w);
    bool cellTooShortForWrap = (h < lineHeight * 2);

    if (fitsWidth || cellTooShortForWrap) {
        int32_t textX = x;
        int32_t textY = y;

        switch (justificationX) {
            case JUSTIFY_LEFT:     textX = x; break;
            case JUSTIFY_CENTER_X: textX = x + (w - (int32_t)tw) / 2; break;
            case JUSTIFY_RIGHT:    textX = x + w - (int32_t)tw; break;
        }

        // GFX text y is baseline position
        switch (justificationY) {
            case JUSTIFY_TOP:      textY = y - y1; break;
            case JUSTIFY_CENTER_Y: textY = y + (h - (int32_t)th) / 2 - y1; break;
            case JUSTIFY_BOTTOM:   textY = y + h - (int32_t)th - y1; break;
        }

        gfx->setCursor(textX, textY);
        gfx->print(m_text);
        return;
    }

    // Multi-line word-wrap case (cell tall enough for at least 2 lines)
    // lineHeight already computed above
    int32_t lineY = y - y1;  // Start from top, baseline-adjusted

    const char* ptr = m_text;
    char lineBuf[128];

    while (*ptr != '\0' && lineY + (int32_t)th <= y + h) {
        int remaining = (int)strlen(ptr);
        int fitLen = remaining;

        // Binary search for max chars that fit in width
        for (int tryLen = remaining; tryLen > 0; tryLen--) {
            int copyLen = (tryLen < 127) ? tryLen : 127;
            memcpy(lineBuf, ptr, copyLen);
            lineBuf[copyLen] = '\0';

            uint16_t testW, testH;
            int16_t tx1, ty1;
            gfx->getTextBounds(lineBuf, 0, 0, &tx1, &ty1, &testW, &testH);

            if ((int32_t)testW <= w) {
                fitLen = tryLen;
                break;
            }
        }

        // Word-boundary break (if we're not at end of string)
        if (fitLen < remaining) {
            int breakAt = fitLen;
            for (int j = fitLen; j > 0; j--) {
                if (ptr[j] == ' ') { breakAt = j; break; }
            }
            fitLen = breakAt;
        }

        // Copy and render this line
        int copyLen = (fitLen < 127) ? fitLen : 127;
        memcpy(lineBuf, ptr, copyLen);
        lineBuf[copyLen] = '\0';

        uint16_t lineW, lineH;
        int16_t lx1, ly1;
        gfx->getTextBounds(lineBuf, 0, 0, &lx1, &ly1, &lineW, &lineH);

        int32_t lx = x;
        switch (justificationX) {
            case JUSTIFY_LEFT:     lx = x; break;
            case JUSTIFY_CENTER_X: lx = x + (w - (int32_t)lineW) / 2; break;
            case JUSTIFY_RIGHT:    lx = x + w - (int32_t)lineW; break;
        }

        gfx->setCursor(lx, lineY);
        gfx->print(lineBuf);

        ptr += fitLen;
        while (*ptr == ' ') ptr++;  // Skip whitespace between lines
        lineY += lineHeight;
    }
}
