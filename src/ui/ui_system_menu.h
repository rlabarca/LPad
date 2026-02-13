/**
 * @file ui_system_menu.h
 * @brief System Menu UI Component (v0.72 - Widget-based)
 *
 * Global transient overlay providing system information (version, WiFi SSID)
 * and interactive WiFi selection via the Widget System.
 * Activated by EDGE_DRAG TOP, dismissed by EDGE_DRAG BOTTOM.
 *
 * Specification: features/ui_system_menu.md
 * Architecture:  features/arch_ui_widgets.md
 */

#ifndef UI_SYSTEM_MENU_H
#define UI_SYSTEM_MENU_H

#include <stdint.h>
#include "widgets/wifi_list_widget.h"

// Forward declarations
class Arduino_GFX;
class Arduino_Canvas;
class RelativeDisplay;
class WidgetLayoutEngine;
class GridWidgetLayout;
class TextWidget;

class SystemMenu {
public:
    enum State {
        CLOSED,
        OPENING,
        OPEN,
        CLOSING
    };

    SystemMenu();
    ~SystemMenu();

    bool begin(Arduino_GFX* gfx, int32_t width, int32_t height);

    void setVersion(const char* version);
    void setSSID(const char* ssid);

    void setBackgroundColor(uint16_t color);
    void setRevealColor(uint16_t color);
    void setVersionFont(const void* font);
    void setVersionColor(uint16_t color);
    void setSSIDFont(const void* font);
    void setSSIDColor(uint16_t color);

    // Widget configuration
    void setHeadingFont(const void* font);
    void setHeadingColor(uint16_t color);
    void setHeadingUnderlined(bool underlined);
    void setListFont(const void* font);

    /** Configure WiFi entries for the WiFiListWidget. */
    void setWiFiEntries(const WiFiListWidget::WiFiEntry* entries, int count);

    /** Set widget theme colors (avoids theme_manager.h in widget code). */
    void setWidgetColors(uint16_t normalText, uint16_t highlight,
                         uint16_t connectingBg, uint16_t errorText,
                         uint16_t scrollIndicator);

    void open();
    void close();

    State getState() const { return m_state; }
    bool isActive() const { return m_state != CLOSED; }

    void update(float deltaTime);
    void render();
    bool handleInput(const touch_gesture_event_t& event);

private:
    Arduino_GFX* m_gfx;
    int32_t m_width;
    int32_t m_height;

    State m_state;
    float m_progress;  // 0.0 = closed, 1.0 = open

    // Content
    const char* m_versionText;
    const char* m_ssidText;

    // Theme
    uint16_t m_bgColor;
    uint16_t m_revealColor;
    const void* m_versionFont;
    uint16_t m_versionColor;
    const void* m_ssidFont;
    uint16_t m_ssidColor;

    // Off-screen canvas for flicker-free rendering (PSRAM)
    Arduino_Canvas* m_canvas;
    RelativeDisplay* m_relDisplay;
    uint16_t* m_canvasBuffer;

    // Widget System
    WidgetLayoutEngine* m_widgetEngine;
    GridWidgetLayout* m_gridLayout;
    TextWidget* m_headingWidget;
    WiFiListWidget* m_wifiList;

    // Dirty tracking
    bool m_dirty;

    // Layout constants (relative coordinates, 0-100%)
    static constexpr float MARGIN_PERCENT = 1.0f;
    static constexpr float SSID_Y_PERCENT = 1.0f;
    static constexpr float VERSION_Y_BOTTOM = 99.0f;
    static constexpr float ANIMATION_DURATION = 0.25f;  // 250ms

    // SSID change callback (wired to WiFiListWidget)
    static void onWiFiSSIDChanged(const char* ssid, void* context);
};

#endif // UI_SYSTEM_MENU_H
