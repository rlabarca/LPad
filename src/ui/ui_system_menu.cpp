/**
 * @file ui_system_menu.cpp
 * @brief System Menu UI Component Implementation (v0.72 - Widget-based)
 *
 * Renders to an off-screen PSRAM canvas via RelativeDisplay, then blits to
 * display in a single DMA transfer for flicker-free animation.
 *
 * Central content (heading + WiFi list) is managed by the Widget System.
 * Legacy SSID and version overlays remain as direct GFX draws.
 *
 * Specification: features/ui_system_menu.md
 * Architecture:  features/arch_ui_widgets.md
 */

#include "ui_system_menu.h"
#include "../relative_display.h"
#include "../themes/default/theme_colors.h"
#include "widgets/ui_widget.h"
#include "widgets/text_widget.h"
#include "widgets/wifi_list_widget.h"
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
    , m_widgetEngine(nullptr)
    , m_gridLayout(nullptr)
    , m_headingWidget(nullptr)
    , m_wifiList(nullptr)
    , m_dirty(false)
{
}

SystemMenu::~SystemMenu() {
    delete m_widgetEngine;
    delete m_gridLayout;
    delete m_headingWidget;
    delete m_wifiList;
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

    // --- Widget System Setup ---
    // GridWidgetLayout: 1 column x 5 rows
    // Anchored at TOP_CENTER, 10% offset down from screen TOP_CENTER
    // Size: 50% width, 50% height
    m_gridLayout = new GridWidgetLayout(5, 1);
    m_gridLayout->setAnchorPoint(ANCHOR_TOP_CENTER);
    m_gridLayout->setScreenRefPoint(ANCHOR_TOP_CENTER);
    m_gridLayout->setOffset(0.0f, 0.10f);  // 10% down
    m_gridLayout->setSize(0.50f, 0.50f);   // 50% x 50%

    // Heading widget (Row 0): "WiFi Networks"
    m_headingWidget = new TextWidget();
    m_headingWidget->setText("WiFi Networks");
    m_headingWidget->justificationX = JUSTIFY_LEFT;
    m_headingWidget->justificationY = JUSTIFY_CENTER_Y;
    m_gridLayout->addWidget(m_headingWidget, 0, 0);

    // WiFi list widget (Rows 1-4, spanning 4 rows)
    m_wifiList = new WiFiListWidget();
    m_wifiList->setSSIDChangeCallback(onWiFiSSIDChanged, this);
    m_gridLayout->addWidget(m_wifiList, 1, 0, 4, 1);

    // Widget layout engine
    m_widgetEngine = new WidgetLayoutEngine();
    m_widgetEngine->addLayout(m_gridLayout);

    // Calculate initial layout
    m_widgetEngine->calculateLayouts(width, height);

    Serial.printf("[SystemMenu] Widget-based canvas + RelativeDisplay: %dx%d\n", width, height);
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

void SystemMenu::setHeadingFont(const void* font) {
    if (m_headingWidget) {
        m_headingWidget->setFont(static_cast<const GFXfont*>(font));
    }
    m_dirty = true;
}

void SystemMenu::setHeadingColor(uint16_t color) {
    if (m_headingWidget) {
        m_headingWidget->setColor(color);
    }
    m_dirty = true;
}

void SystemMenu::setHeadingUnderlined(bool underlined) {
    if (m_headingWidget) {
        m_headingWidget->setUnderlined(underlined);
    }
    m_dirty = true;
}

void SystemMenu::setListFont(const void* font) {
    if (m_wifiList) {
        m_wifiList->setFont(font);
    }
    m_dirty = true;
}

void SystemMenu::setWiFiEntries(const WiFiListWidget::WiFiEntry* entries, int count) {
    if (m_wifiList) {
        m_wifiList->setEntries(entries, count);
    }
    m_dirty = true;
}

void SystemMenu::setWidgetColors(uint16_t normalText, uint16_t highlight,
                                  uint16_t connectingBg, uint16_t errorText,
                                  uint16_t scrollIndicator) {
    if (m_wifiList) {
        m_wifiList->setNormalColor(normalText);
        m_wifiList->setHighlightColor(highlight);
        m_wifiList->setConnectingBgColor(connectingBg);
        m_wifiList->setErrorColor(errorText);
        m_wifiList->setScrollIndicatorColor(scrollIndicator);
    }
    // Heading color is set independently via setHeadingColor() per spec
    m_dirty = true;
}

void SystemMenu::open() {
    if (m_state == CLOSED) {
        m_state = OPENING;
        m_progress = 0.0f;
        m_dirty = true;

        // Recalculate layout on every open (handles orientation changes)
        if (m_widgetEngine) {
            m_widgetEngine->calculateLayouts(m_width, m_height);
        }

        // Refresh WiFi list status
        if (m_wifiList) {
            m_wifiList->refresh();
        }
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

    // Poll widget updates while visible (blink animation + WiFi status)
    if (m_state != CLOSED && m_widgetEngine) {
        m_widgetEngine->update();
        m_dirty = true;
    }
}

void SystemMenu::render() {
    if (m_state == CLOSED || m_canvas == nullptr || m_canvasBuffer == nullptr) return;
    if (!m_dirty) return;

    // Convert animation progress to relative height (0-100%)
    float visiblePercent = m_progress * 100.0f;
    if (visiblePercent <= 0.0f) return;
    if (visiblePercent > 100.0f) visiblePercent = 100.0f;

    // Fill visible menu area with background color
    m_relDisplay->fillRect(0.0f, 0.0f, 100.0f, visiblePercent, m_bgColor);

    // Fill exposed area below menu with reveal color for smooth animation
    if (visiblePercent < 100.0f) {
        m_relDisplay->fillRect(0.0f, visiblePercent, 100.0f, 100.0f - visiblePercent, m_revealColor);
    }

    // Absolute visible height for clipping
    int32_t visiblePx = m_relDisplay->relativeToAbsoluteHeight(visiblePercent);

    // --- Render Widget System (heading + WiFi list) ---
    // Spec: NO widgets during OPENING/CLOSING; only visible once fully OPEN
    if (m_state == OPEN && m_widgetEngine) {
        m_widgetEngine->render(m_canvas, visiblePx);
    }

    // Legacy overlays also only drawn when fully OPEN (same as widgets)
    if (m_state == OPEN) {
        // --- Legacy SSID overlay (top-right corner) ---
        if (m_ssidText != nullptr && m_ssidText[0] != '\0') {
            m_canvas->setFont(static_cast<const GFXfont*>(m_ssidFont));
            m_canvas->setTextColor(m_ssidColor);

            int16_t x1, y1;
            uint16_t tw, th;
            m_canvas->getTextBounds(m_ssidText, 0, 0, &x1, &y1, &tw, &th);

            int32_t text_y = m_relDisplay->relativeToAbsoluteY(SSID_Y_PERCENT) - y1;
            int32_t right_edge = m_relDisplay->relativeToAbsoluteX(100.0f - MARGIN_PERCENT);
            int32_t text_x = right_edge - static_cast<int32_t>(tw);

            if (text_y + y1 >= 0 && text_y + y1 + static_cast<int32_t>(th) <= visiblePx) {
                m_canvas->setCursor(text_x, text_y);
                m_canvas->print(m_ssidText);
            }
        }

        // --- Legacy version overlay (bottom-center) ---
        if (m_versionText != nullptr && m_versionText[0] != '\0') {
            m_canvas->setFont(static_cast<const GFXfont*>(m_versionFont));
            m_canvas->setTextColor(m_versionColor);

            int16_t x1, y1;
            uint16_t tw, th;
            m_canvas->getTextBounds(m_versionText, 0, 0, &x1, &y1, &tw, &th);

            int32_t bottom_edge = m_relDisplay->relativeToAbsoluteY(VERSION_Y_BOTTOM);
            int32_t text_y = bottom_edge - th - y1;
            int32_t text_x = (m_width - static_cast<int32_t>(tw)) / 2;

            if (text_y + y1 >= 0 && text_y + y1 + static_cast<int32_t>(th) <= visiblePx) {
                m_canvas->setCursor(text_x, text_y);
                m_canvas->print(m_versionText);
            }
        }
    }

    // Single atomic DMA blit — entire frame appears at once, no flicker
    hal_display_fast_blit(0, 0, static_cast<int16_t>(m_width),
                          static_cast<int16_t>(m_height), m_canvasBuffer);

    m_dirty = false;
}

bool SystemMenu::handleInput(const touch_gesture_event_t& event) {
    if (m_state != OPEN || m_widgetEngine == nullptr) return false;
    return m_widgetEngine->handleInput(event);
}

void SystemMenu::onWiFiSSIDChanged(const char* ssid, void* context) {
    SystemMenu* self = static_cast<SystemMenu*>(context);
    if (self) {
        self->setSSID(ssid);
    }
}
