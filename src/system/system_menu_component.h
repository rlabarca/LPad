/**
 * @file system_menu_component.h
 * @brief System Menu SystemComponent (Z=20) - v0.72 Widget-based
 *
 * Wraps SystemMenu as a managed SystemComponent with activation events,
 * close animation detection, systemPause lifecycle, and widget-based
 * WiFi selection.
 */

#ifndef SYSTEM_MENU_COMPONENT_H
#define SYSTEM_MENU_COMPONENT_H

#include "../ui/ui_component.h"
#include "../ui/widgets/wifi_list_widget.h"
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

    // Widget configuration (v0.72)
    void setHeadingFont(const void* font);
    void setHeadingColor(uint16_t color);
    void setHeadingUnderlined(bool underlined);
    void setListFont(const void* font);
    void setWiFiEntries(const WiFiListWidget::WiFiEntry* entries, int count);
    void setWidgetColors(uint16_t normalText, uint16_t highlight,
                         uint16_t connectingBg, uint16_t errorText,
                         uint16_t scrollIndicator);

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
