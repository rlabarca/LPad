/**
 * @file ui_touch_test_overlay.cpp
 * @brief Touch Test Overlay UI Component Implementation
 */

#ifndef UNIT_TEST  // Only compile for target hardware

#include "ui_touch_test_overlay.h"
#include "theme_manager.h"
#include "../hal/display.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <cstdio>
#include <cstring>

TouchTestOverlay::TouchTestOverlay()
    : m_visible(false),
      m_time_since_last_event_ms(0),
      m_last_type(TOUCH_NONE),
      m_last_direction(TOUCH_DIR_NONE),
      m_last_x(0),
      m_last_y(0),
      m_last_x_percent(0.0f),
      m_last_y_percent(0.0f),
      m_text_buffer(nullptr),
      m_text_width(0),
      m_text_height(0),
      m_buffer_valid(false)
{
}

bool TouchTestOverlay::begin() {
    // Allocate buffer for text rendering
    // Use a reasonable size for text overlay (200x60 pixels)
    m_text_width = 200;
    m_text_height = 60;

    m_text_buffer = static_cast<uint16_t*>(malloc(m_text_width * m_text_height * sizeof(uint16_t)));
    if (!m_text_buffer) {
        Serial.println("[TouchTestOverlay] Failed to allocate text buffer");
        return false;
    }

    return true;
}

void TouchTestOverlay::update(const touch_gesture_event_t& event) {
    // Store event data
    m_last_type = event.type;
    m_last_direction = event.direction;
    m_last_x = event.x_px;
    m_last_y = event.y_px;
    m_last_x_percent = event.x_percent;
    m_last_y_percent = event.y_percent;

    // Show overlay and reset timeout
    m_visible = true;
    m_time_since_last_event_ms = 0;
    m_buffer_valid = false;  // Invalidate cached render
}

void TouchTestOverlay::tick(uint32_t delta_time) {
    if (m_visible) {
        m_time_since_last_event_ms += delta_time;

        // Auto-hide after timeout
        if (m_time_since_last_event_ms >= TIMEOUT_MS) {
            m_visible = false;
        }
    }
}

void TouchTestOverlay::render() {
    if (!m_visible || !m_text_buffer) {
        return;
    }

    // Render text to buffer if needed
    if (!m_buffer_valid) {
        renderTextToBuffer();
        m_buffer_valid = true;
    }

    // Calculate centered position
    int32_t screen_width = hal_display_get_width_pixels();
    int32_t screen_height = hal_display_get_height_pixels();
    int16_t x = (screen_width - m_text_width) / 2;
    int16_t y = (screen_height - m_text_height) / 2;

    // Blit with transparency (chroma key 0x0001)
    hal_display_fast_blit_transparent(x, y, m_text_width, m_text_height, m_text_buffer, 0x0001);
}

void TouchTestOverlay::renderTextToBuffer() {
    // Get theme
    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();

    // Create a temporary canvas for rendering (standalone, no parent)
    Arduino_Canvas canvas(m_text_width, m_text_height, nullptr);

    // Fill with chroma key for transparency
    constexpr uint16_t CHROMA_KEY = 0x0001;
    canvas.fillScreen(CHROMA_KEY);

    // Set text properties (second largest font per spec)
    canvas.setFont(theme->fonts.ui);  // 18pt UI font
    canvas.setTextColor(RGB565_WHITE);
    canvas.setTextSize(1);

    // Build text string
    char text_line1[64];
    char text_line2[64];

    const char* gesture_name = gestureTypeToString(m_last_type);
    const char* dir_name = directionToString(m_last_direction);

    if (m_last_direction != TOUCH_DIR_NONE) {
        // Gesture with direction (swipe, edge drag)
        snprintf(text_line1, sizeof(text_line1), "%s: %s", gesture_name, dir_name);
        snprintf(text_line2, sizeof(text_line2), "(%d, %d) %.0f%%", m_last_x, m_last_y, m_last_x_percent * 100.0f);
    } else {
        // Gesture without direction (tap, hold, hold_drag)
        snprintf(text_line1, sizeof(text_line1), "%s", gesture_name);
        snprintf(text_line2, sizeof(text_line2), "(%d, %d) %.0f%%", m_last_x, m_last_y, m_last_x_percent * 100.0f);
    }

    // Draw background box for legibility
    int16_t text1_x, text1_y;
    uint16_t text1_w, text1_h;
    canvas.getTextBounds(text_line1, 0, 0, &text1_x, &text1_y, &text1_w, &text1_h);

    int16_t text2_x, text2_y;
    uint16_t text2_w, text2_h;
    canvas.getTextBounds(text_line2, 0, 0, &text2_x, &text2_y, &text2_w, &text2_h);

    uint16_t max_w = (text1_w > text2_w) ? text1_w : text2_w;
    uint16_t total_h = text1_h + text2_h + 10;  // 10px spacing

    int16_t box_x = (m_text_width - max_w) / 2 - 5;
    int16_t box_y = (m_text_height - total_h) / 2 - 5;
    uint16_t box_w = max_w + 10;
    uint16_t box_h = total_h + 10;

    canvas.fillRect(box_x, box_y, box_w, box_h, theme->colors.background);

    // Draw text (centered)
    int16_t text1_pos_x = (m_text_width - text1_w) / 2;
    int16_t text1_pos_y = (m_text_height - total_h) / 2;
    canvas.setCursor(text1_pos_x, text1_pos_y);
    canvas.print(text_line1);

    int16_t text2_pos_x = (m_text_width - text2_w) / 2;
    int16_t text2_pos_y = text1_pos_y + text1_h + 5;
    canvas.setCursor(text2_pos_x, text2_pos_y);
    canvas.print(text_line2);

    // Copy framebuffer to cached buffer
    memcpy(m_text_buffer, canvas.getFramebuffer(), m_text_width * m_text_height * sizeof(uint16_t));
}

const char* TouchTestOverlay::gestureTypeToString(touch_gesture_type_t type) const {
    switch (type) {
        case TOUCH_TAP: return "TAP";
        case TOUCH_HOLD: return "HOLD";
        case TOUCH_HOLD_DRAG: return "HOLD_DRAG";
        case TOUCH_SWIPE: return "SWIPE";
        case TOUCH_EDGE_DRAG: return "EDGE_DRAG";
        default: return "NONE";
    }
}

const char* TouchTestOverlay::directionToString(touch_direction_t dir) const {
    switch (dir) {
        case TOUCH_DIR_UP: return "UP";
        case TOUCH_DIR_DOWN: return "DOWN";
        case TOUCH_DIR_LEFT: return "LEFT";
        case TOUCH_DIR_RIGHT: return "RIGHT";
        default: return "";
    }
}

#endif // !UNIT_TEST
