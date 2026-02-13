/**
 * @file ui_system_menu.cpp
 * @brief System Menu UI Component Implementation
 *
 * Renders to an off-screen PSRAM canvas via RelativeDisplay, then blits to
 * display in a single DMA transfer for flicker-free animation.
 * All layout uses relative 0-100% coordinates per ARCHITECTURE.md §E.1.
 */

#include "ui_system_menu.h"
#include "../relative_display.h"
#include "../themes/default/theme_colors.h"
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
    , m_bgColor(LPad::THEME_SYSTEM_MENU_BG)
    , m_revealColor(LPad::THEME_BACKGROUND)
    , m_versionFont(nullptr)
    , m_versionColor(LPad::THEME_TEXT_VERSION)
    , m_ssidFont(nullptr)
    , m_ssidColor(LPad::THEME_TEXT_STATUS)
    , m_canvas(nullptr)
    , m_relDisplay(nullptr)
    , m_canvasBuffer(nullptr)
    , m_dirty(false)
{
}

SystemMenu::~SystemMenu() {
    delete m_relDisplay;
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

    // Wrap canvas in RelativeDisplay for 0-100% coordinate drawing
    m_relDisplay = new RelativeDisplay(m_canvas, width, height);
    m_relDisplay->init();

    Serial.printf("[SystemMenu] Canvas + RelativeDisplay created: %dx%d\n", width, height);
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

    // Convert animation progress to relative height (0-100%)
    float visiblePercent = m_progress * 100.0f;
    if (visiblePercent <= 0.0f) return;
    if (visiblePercent > 100.0f) visiblePercent = 100.0f;

    // Fill visible menu area with background color (relative coordinates)
    m_relDisplay->fillRect(0.0f, 0.0f, 100.0f, visiblePercent, m_bgColor);

    // Fill exposed area below menu with reveal color for smooth animation
    if (visiblePercent < 100.0f) {
        m_relDisplay->fillRect(0.0f, visiblePercent, 100.0f, 100.0f - visiblePercent, m_revealColor);
    }

    // Absolute visible height for text clipping
    int32_t visiblePx = m_relDisplay->relativeToAbsoluteHeight(visiblePercent);

    // Draw SSID text (top-right corner)
    if (m_ssidText != nullptr && m_ssidText[0] != '\0') {
        m_canvas->setFont(static_cast<const GFXfont*>(m_ssidFont));
        m_canvas->setTextColor(m_ssidColor);

        int16_t x1, y1;
        uint16_t tw, th;
        m_canvas->getTextBounds(m_ssidText, 0, 0, &x1, &y1, &tw, &th);

        // Position: MARGIN_PERCENT from top, MARGIN_PERCENT from right edge
        int32_t text_y = m_relDisplay->relativeToAbsoluteY(SSID_Y_PERCENT) - y1;
        int32_t right_edge = m_relDisplay->relativeToAbsoluteX(100.0f - MARGIN_PERCENT);
        int32_t text_x = right_edge - static_cast<int32_t>(tw);

        if (text_y + y1 >= 0 && text_y + y1 + static_cast<int32_t>(th) <= visiblePx) {
            m_canvas->setCursor(text_x, text_y);
            m_canvas->print(m_ssidText);
        }
    }

    // Draw version text (bottom-center)
    if (m_versionText != nullptr && m_versionText[0] != '\0') {
        m_canvas->setFont(static_cast<const GFXfont*>(m_versionFont));
        m_canvas->setTextColor(m_versionColor);

        int16_t x1, y1;
        uint16_t tw, th;
        m_canvas->getTextBounds(m_versionText, 0, 0, &x1, &y1, &tw, &th);

        // Position: centered horizontally, VERSION_Y_BOTTOM% from top (bottom edge of text)
        int32_t bottom_edge = m_relDisplay->relativeToAbsoluteY(VERSION_Y_BOTTOM);
        int32_t text_y = bottom_edge - th - y1;  // Cursor position for bottom alignment
        int32_t text_x = (m_width - static_cast<int32_t>(tw)) / 2;  // Centered

        if (text_y + y1 >= 0 && text_y + y1 + static_cast<int32_t>(th) <= visiblePx) {
            m_canvas->setCursor(text_x, text_y);
            m_canvas->print(m_versionText);
        }
    }

    // Single atomic DMA blit — entire frame appears at once, no flicker
    hal_display_fast_blit(0, 0, static_cast<int16_t>(m_width),
                          static_cast<int16_t>(m_height), m_canvasBuffer);

    m_dirty = false;
}
