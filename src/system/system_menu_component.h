/**
 * @file system_menu_component.h
 * @brief System Menu SystemComponent (Z=20)
 *
 * Wraps SystemMenu as a managed SystemComponent with activation events,
 * close animation detection, and systemPause lifecycle.
 */

#ifndef SYSTEM_MENU_COMPONENT_H
#define SYSTEM_MENU_COMPONENT_H

#include "../ui/ui_component.h"
#include <stdint.h>

class Arduino_GFX;
class SystemMenu;

class SystemMenuComponent : public SystemComponent {
public:
    SystemMenuComponent();
    ~SystemMenuComponent();

    bool begin(Arduino_GFX* gfx, int32_t width, int32_t height);

    // Configuration (call after begin, before first use)
    void setVersion(const char* version);
    void setSSID(const char* ssid);
    void setBackgroundColor(uint16_t color);
    void setRevealColor(uint16_t color);
    void setVersionFont(const void* font);
    void setVersionColor(uint16_t color);
    void setSSIDFont(const void* font);
    void setSSIDColor(uint16_t color);

    /** Set a callback that returns the current SSID (called on menu open). */
    typedef const char* (*SSIDProvider)();
    void setSSIDProvider(SSIDProvider fn) { m_ssidProvider = fn; }

    // UIComponent lifecycle
    void onUnpause() override;
    void update(float dt) override;
    void render() override;
    bool handleInput(const touch_gesture_event_t& event) override;

    bool isOpaque() const override { return true; }
    bool isFullscreen() const override { return true; }

private:
    SystemMenu* m_inner;
    bool m_closing;
    SSIDProvider m_ssidProvider = nullptr;
};

#endif // SYSTEM_MENU_COMPONENT_H
