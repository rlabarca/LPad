/**
 * @file ui_system_menu.cpp
 * @brief System Menu UI Component Implementation
 *
 * Renders to an off-screen PSRAM canvas then blits to display in a single
 * DMA transfer, eliminating the flicker caused by drawing directly to the
 * unbuffered AMOLED display driver.
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
    , m_revealColor(0x0000)
    , m_versionFont(nullptr)
    , m_versionColor(0x7BEF)  // Grey
    , m_ssidFont(nullptr)
    , m_ssidColor(0xFFFF)  // White
    , m_canvas(nullptr)
    , m_canvasBuffer(nullptr)
    , m_dirty(false)
{
}

SystemMenu::~SystemMenu() {
    delete m_canvas;
}

bool SystemMenu::begin(Arduino_GFX* gfx, int32_t width, int32_t height) {
    if (gfx == nullptr) return false;
    m_gfx = gfx;
    m_width = width;
    m_height = height;

    // Create off-screen canvas in PSRAM for flicker-free rendering
    m_canvas = new Arduino_Canvas(width, height, nullptr);
    if (m_canvas == nullptr) {
        Serial.println("[SystemMenu] Failed to allocate canvas");
        return false;
    }

    if (!m_canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) {
        Serial.println("[SystemMenu] Failed to initialize canvas");
        delete m_canvas;
        m_canvas = nullptr;
        return false;
    }

    m_canvasBuffer = m_canvas->getFramebuffer();
    if (m_canvasBuffer == nullptr) {
        Serial.println("[SystemMenu] Canvas framebuffer is nullptr");
        delete m_canvas;
        m_canvas = nullptr;
        return false;
    }

    Serial.printf("[SystemMenu] Canvas created: %dx%d in PSRAM\n", width, height);
    return true;
}

void SystemMenu::setVersion(const char* version) {
    m_versionText = version;
    m_dirty = true;
}

void SystemMenu::setSSID(const char* ssid) {
    m_ssidText = ssid;
    m_dirty = true;
}

void SystemMenu::setBackgroundColor(uint16_t color) {
    m_bgColor = color;
    m_dirty = true;
}

void SystemMenu::setRevealColor(uint16_t color) {
    m_revealColor = color;
    m_dirty = true;
}

void SystemMenu::setVersionFont(const void* font) {
    m_versionFont = font;
    m_dirty = true;
}

void SystemMenu::setVersionColor(uint16_t color) {
    m_versionColor = color;
    m_dirty = true;
}

void SystemMenu::setSSIDFont(const void* font) {
    m_ssidFont = font;
    m_dirty = true;
}

void SystemMenu::setSSIDColor(uint16_t color) {
    m_ssidColor = color;
    m_dirty = true;
}

void SystemMenu::open() {
    if (m_state == CLOSED) {
        m_state = OPENING;
        m_progress = 0.0f;
        m_dirty = true;
    }
}

void SystemMenu::close() {
    if (m_state == OPEN) {
        m_state = CLOSING;
        m_progress = 1.0f;
        m_dirty = true;
    }
}

void SystemMenu::update(float deltaTime) {
    float speed = 1.0f / ANIMATION_DURATION;

    switch (m_state) {
        case OPENING:
            m_progress += speed * deltaTime;
            m_dirty = true;
            if (m_progress >= 1.0f) {
                m_progress = 1.0f;
                m_state = OPEN;
            }
            break;

        case CLOSING:
            m_progress -= speed * deltaTime;
            m_dirty = true;
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
    if (m_state == CLOSED || m_canvas == nullptr || m_canvasBuffer == nullptr) return;
    if (!m_dirty) return;

    int32_t visible_height = static_cast<int32_t>(m_progress * m_height);
    if (visible_height <= 0) return;
    if (visible_height > m_height) visible_height = m_height;

    // Draw everything to the off-screen canvas (no display writes yet)

    // Fill visible menu area with background color
    m_canvas->fillRect(0, 0, m_width, visible_height, m_bgColor);

    // Fill exposed area below menu with reveal color for smooth animation
    if (visible_height < m_height) {
        m_canvas->fillRect(0, visible_height, m_width, m_height - visible_height, m_revealColor);
    }

    // Draw version text (bottom-center, small/subtle)
    if (m_versionText != nullptr && m_versionText[0] != '\0') {
        m_canvas->setFont(static_cast<const GFXfont*>(m_versionFont));
        m_canvas->setTextColor(m_versionColor);

        int16_t x1, y1;
        uint16_t tw, th;
        m_canvas->getTextBounds(m_versionText, 0, 0, &x1, &y1, &tw, &th);

        int16_t text_x = (m_width - tw) / 2;  // Centered horizontally
        int16_t text_y = m_height - th - 4 - y1;  // 4px bottom margin, adjust for baseline
        if (text_y >= 0 && text_y + static_cast<int32_t>(th) <= visible_height) {
            m_canvas->setCursor(text_x, text_y);
            m_canvas->print(m_versionText);
        }
    }

    // Draw SSID text (top-right, normal)
    if (m_ssidText != nullptr && m_ssidText[0] != '\0') {
        m_canvas->setFont(static_cast<const GFXfont*>(m_ssidFont));
        m_canvas->setTextColor(m_ssidColor);

        int16_t x1, y1;
        uint16_t tw, th;
        m_canvas->getTextBounds(m_ssidText, 0, 0, &x1, &y1, &tw, &th);

        int16_t text_x = m_width - tw - 4;  // Right-aligned with margin
        int16_t text_y = 4 - y1;  // Same top margin as version
        if (text_y + static_cast<int32_t>(th) <= visible_height) {
            m_canvas->setCursor(text_x, text_y);
            m_canvas->print(m_ssidText);
        }
    }

    // Single atomic DMA blit — entire frame appears at once, no flicker
    hal_display_fast_blit(0, 0, static_cast<int16_t>(m_width),
                          static_cast<int16_t>(m_height), m_canvasBuffer);

    m_dirty = false;
}
