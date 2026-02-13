/**
 * @file wifi_list_widget.h
 * @brief WiFiListWidget - specialized ScrollableListWidget for WiFi management
 *
 * Specification: features/ui_wifi_list_widget.md
 *
 * IMPORTANT: This file does NOT include theme_manager.h.
 * Colors are configured externally to avoid pulling in font data arrays
 * that cause watchdog crashes during rendering (see MEMORY.md).
 */

#ifndef WIFI_LIST_WIDGET_H
#define WIFI_LIST_WIDGET_H

#include "scrollable_list_widget.h"

class WiFiListWidget : public ScrollableListWidget {
public:
    /** WiFi credential entry (matches inject_config.py output format). */
    struct WiFiEntry {
        const char* ssid;
        const char* password;
    };

    WiFiListWidget();

    /** Populate list from compiled WiFi config array. */
    void setEntries(const WiFiEntry* entries, int count);

    /** Re-check current network status and update highlights. */
    void refresh();

    /** Poll hal_network_get_status() and update visual state. */
    void update() override;

    // Theme colors (set externally, NOT from theme_manager.h)
    void setHighlightColor(uint16_t color) { m_highlightColor = color; }
    void setConnectingBgColor(uint16_t color) { m_connectingBgColor = color; }
    void setErrorColor(uint16_t color) { m_errorColor = color; }
    void setNormalColor(uint16_t color) { m_normalColor = color; }

    /** Callback fired when connection succeeds (for SSID display update). */
    using SSIDChangeCallback = void(*)(const char* ssid, void* context);
    void setSSIDChangeCallback(SSIDChangeCallback cb, void* ctx = nullptr) {
        m_ssidChangeCb = cb;
        m_ssidChangeCtx = ctx;
    }

    int getActiveIndex() const { return m_activeIndex; }
    int getConnectingIndex() const { return m_connectingIndex; }

private:
    const WiFiEntry* m_entries = nullptr;
    int m_entryCount = 0;
    int m_connectingIndex = -1;
    int m_activeIndex = -1;

    uint16_t m_highlightColor = 0x8D51;    // Default: SAGE
    uint16_t m_connectingBgColor = 0x4268;  // Default: FOREST
    uint16_t m_errorColor = 0xF800;         // Default: RED
    uint16_t m_normalColor = 0xBED6;        // Default: CREAM

    SSIDChangeCallback m_ssidChangeCb = nullptr;
    void* m_ssidChangeCtx = nullptr;

    // Blink state for connecting animation (0.75s period per spec)
    bool m_blinkOn = false;
    unsigned long m_lastBlinkMs = 0;
    static constexpr unsigned long BLINK_INTERVAL_MS = 750;

    static void onItemSelected(int index, void* context);
    void handleSelection(int index);
};

#endif // WIFI_LIST_WIDGET_H
