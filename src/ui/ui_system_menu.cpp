/**
 * @file ui_system_menu.cpp
 * @brief System Menu UI Component Implementation
 */

#include "ui_system_menu.h"
#include <Arduino_GFX_Library.h>
#include "../../hal/display.h"

SystemMenu::SystemMenu()
    : m_gfx(nullptr)
    , m_width(0)
    , m_height(0)
    , m_state(CLOSED)
    , m_progress(0.0f)
    , m_versionText(nullptr)
    , m_ssidText(nullptr)
    , m_bgColor(0x0000)  // Black
    , m_versionFont(nullptr)
    , m_versionColor(0x7BEF)  // Grey
    , m_ssidFont(nullptr)
    , m_ssidColor(0xFFFF)  // White
{
}

SystemMenu::~SystemMenu() {
}

bool SystemMenu::begin(Arduino_GFX* gfx, int32_t width, int32_t height) {
    if (gfx == nullptr) return false;
    m_gfx = gfx;
    m_width = width;
    m_height = height;
    return true;
}

void SystemMenu::setVersion(const char* version) {
    m_versionText = version;
}

void SystemMenu::setSSID(const char* ssid) {
    m_ssidText = ssid;
}

void SystemMenu::setBackgroundColor(uint16_t color) {
    m_bgColor = color;
}

void SystemMenu::setVersionFont(const void* font) {
    m_versionFont = font;
}

void SystemMenu::setVersionColor(uint16_t color) {
    m_versionColor = color;
}

void SystemMenu::setSSIDFont(const void* font) {
    m_ssidFont = font;
}

void SystemMenu::setSSIDColor(uint16_t color) {
    m_ssidColor = color;
}

void SystemMenu::open() {
    if (m_state == CLOSED) {
        m_state = OPENING;
        m_progress = 0.0f;
    }
}

void SystemMenu::close() {
    if (m_state == OPEN) {
        m_state = CLOSING;
        m_progress = 1.0f;
    }
}

void SystemMenu::update(float deltaTime) {
    float speed = 1.0f / ANIMATION_DURATION;

    switch (m_state) {
        case OPENING:
            m_progress += speed * deltaTime;
            if (m_progress >= 1.0f) {
                m_progress = 1.0f;
                m_state = OPEN;
            }
            break;

        case CLOSING:
            m_progress -= speed * deltaTime;
            if (m_progress <= 0.0f) {
                m_progress = 0.0f;
                m_state = CLOSED;
            }
            break;

        case OPEN:
        case CLOSED:
            break;
    }
}

void SystemMenu::render() {
    if (m_state == CLOSED || m_gfx == nullptr) return;

    int32_t visible_height = static_cast<int32_t>(m_progress * m_height);
    if (visible_height <= 0) return;
    if (visible_height > m_height) visible_height = m_height;

    // Fill visible area with background color
    m_gfx->fillRect(0, 0, m_width, visible_height, m_bgColor);

    // Draw version text (top-left, small/subtle)
    if (m_versionText != nullptr && m_versionText[0] != '\0') {
        m_gfx->setFont(static_cast<const GFXfont*>(m_versionFont));
        m_gfx->setTextColor(m_versionColor);

        int16_t x1, y1;
        uint16_t tw, th;
        m_gfx->getTextBounds(m_versionText, 0, 0, &x1, &y1, &tw, &th);

        int16_t text_y = 4 - y1;  // Small top margin, adjust for baseline
        if (text_y + th <= visible_height) {
            m_gfx->setCursor(4, text_y);
            m_gfx->print(m_versionText);
        }
    }

    // Draw SSID text (top-right, normal)
    if (m_ssidText != nullptr && m_ssidText[0] != '\0') {
        m_gfx->setFont(static_cast<const GFXfont*>(m_ssidFont));
        m_gfx->setTextColor(m_ssidColor);

        int16_t x1, y1;
        uint16_t tw, th;
        m_gfx->getTextBounds(m_ssidText, 0, 0, &x1, &y1, &tw, &th);

        int16_t text_x = m_width - tw - 4;  // Right-aligned with margin
        int16_t text_y = 4 - y1;  // Same top margin as version
        if (text_y + th <= visible_height) {
            m_gfx->setCursor(text_x, text_y);
            m_gfx->print(m_ssidText);
        }
    }

    hal_display_flush();
}
