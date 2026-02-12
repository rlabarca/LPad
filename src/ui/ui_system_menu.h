/**
 * @file ui_system_menu.h
 * @brief System Menu UI Component
 *
 * Global transient overlay providing system information (version, WiFi SSID).
 * Activated by EDGE_DRAG TOP, dismissed by EDGE_DRAG BOTTOM.
 * While active, suppresses all underlying application rendering.
 *
 * Specification: features/ui_system_menu.md
 */

#ifndef UI_SYSTEM_MENU_H
#define UI_SYSTEM_MENU_H

#include <stdint.h>

// Forward declaration (Arduino_GFX is a class, safe to forward-declare)
class Arduino_GFX;

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
    void setVersionFont(const void* font);
    void setVersionColor(uint16_t color);
    void setSSIDFont(const void* font);
    void setSSIDColor(uint16_t color);

    void open();
    void close();

    State getState() const { return m_state; }
    bool isActive() const { return m_state != CLOSED; }

    void update(float deltaTime);
    void render();

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
    const void* m_versionFont;
    uint16_t m_versionColor;
    const void* m_ssidFont;
    uint16_t m_ssidColor;

    static constexpr float ANIMATION_DURATION = 0.25f;  // 250ms
};

#endif // UI_SYSTEM_MENU_H
