/**
 * @file wifi_list_widget.cpp
 * @brief WiFiListWidget implementation
 *
 * Specification: features/ui_wifi_list_widget.md
 * Implementation Notes:
 *   - Polls hal_network_get_status() during update() while connecting
 *   - Stored credentials accessed via compiled LPAD_WIFI_CONFIG array
 *   - Colors set externally to avoid theme_manager.h inclusion
 */

#include "wifi_list_widget.h"
#include "../../../hal/network.h"
#include <string.h>
#ifdef ARDUINO
#include <Arduino.h>
#else
// Native test fallback
static unsigned long s_fake_millis = 0;
static unsigned long millis() { return s_fake_millis; }
#endif

WiFiListWidget::WiFiListWidget() {
    setSelectionCallback(onItemSelected, this);
    setCirclePosition(CIRCLE_LEFT);
}

void WiFiListWidget::setEntries(const WiFiEntry* entries, int count) {
    m_entries = entries;
    m_entryCount = count;
    clearItems();

    // Check current network to highlight the active one
    const char* currentSSID = hal_network_get_ssid();
    hal_network_status_t status = hal_network_get_status();

    m_activeIndex = -1;
    m_connectingIndex = -1;
    m_failedIndex = -1;

    for (int i = 0; i < count; i++) {
        uint16_t color = m_normalColor;
        if (status == HAL_NETWORK_STATUS_CONNECTED &&
            currentSSID != nullptr &&
            strcmp(entries[i].ssid, currentSSID) == 0) {
            color = m_highlightColor;
            m_activeIndex = i;
        }
        addItem(entries[i].ssid, color);
    }

    // Draw circle on active/connected item
    if (m_activeIndex >= 0) {
        setItemCircle(m_activeIndex, m_highlightColor);
    }
}

void WiFiListWidget::refresh() {
    const char* currentSSID = hal_network_get_ssid();
    hal_network_status_t status = hal_network_get_status();

    m_activeIndex = -1;
    m_connectingIndex = -1;
    m_failedIndex = -1;

    for (int i = 0; i < m_entryCount && i < m_itemCount; i++) {
        if (status == HAL_NETWORK_STATUS_CONNECTED &&
            currentSSID != nullptr &&
            strcmp(m_entries[i].ssid, currentSSID) == 0) {
            setItemColor(i, m_highlightColor);
            setItemCircle(i, m_highlightColor);
            m_activeIndex = i;
        } else {
            setItemColor(i, m_normalColor);
            clearItemCircle(i);
        }
        clearItemBackground(i);
    }
}

void WiFiListWidget::onItemSelected(int index, void* context) {
    WiFiListWidget* self = static_cast<WiFiListWidget*>(context);
    if (self) self->handleSelection(index);
}

void WiFiListWidget::handleSelection(int index) {
    if (index < 0 || index >= m_entryCount) return;
    if (index == m_activeIndex) return;  // Already connected to this one

    // Reset previous failed item to normal (spec: Red/Failed must return to normal immediately)
    if (m_failedIndex >= 0) {
        setItemColor(m_failedIndex, m_normalColor);
        clearItemBackground(m_failedIndex);
        m_failedIndex = -1;
    }

    // Reset previous active item to normal and remove circle
    if (m_activeIndex >= 0) {
        setItemColor(m_activeIndex, m_normalColor);
        clearItemBackground(m_activeIndex);
        clearItemCircle(m_activeIndex);
    }

    // Reset previous connecting item if different
    if (m_connectingIndex >= 0 && m_connectingIndex != m_activeIndex) {
        setItemColor(m_connectingIndex, m_normalColor);
        clearItemBackground(m_connectingIndex);
    }

    // Mark new item as connecting (blink starts ON)
    m_connectingIndex = index;
    m_blinkOn = true;
    m_lastBlinkMs = millis();
    setItemBackground(index, m_connectingBgColor);
    m_activeIndex = -1;

    // Initiate connection via HAL
    hal_network_init(m_entries[index].ssid, m_entries[index].password);
}

void WiFiListWidget::update() {
    if (m_connectingIndex < 0) return;

    hal_network_status_t status = hal_network_get_status();

    switch (status) {
        case HAL_NETWORK_STATUS_CONNECTED: {
            // Connection succeeded — highlight text + circle, remove blink bg
            clearItemBackground(m_connectingIndex);
            setItemColor(m_connectingIndex, m_highlightColor);
            setItemCircle(m_connectingIndex, m_highlightColor);
            m_activeIndex = m_connectingIndex;
            m_connectingIndex = -1;

            // Notify parent to update SSID display
            if (m_ssidChangeCb && m_activeIndex >= 0) {
                m_ssidChangeCb(m_entries[m_activeIndex].ssid, m_ssidChangeCtx);
            }
            break;
        }

        case HAL_NETWORK_STATUS_ERROR:
        case HAL_NETWORK_STATUS_DISCONNECTED: {
            // Connection failed — red text, track failed index for reset
            if (m_connectingIndex >= 0) {
                clearItemBackground(m_connectingIndex);
                setItemColor(m_connectingIndex, m_errorColor);
                m_failedIndex = m_connectingIndex;
                m_connectingIndex = -1;
            }
            break;
        }

        case HAL_NETWORK_STATUS_CONNECTING: {
            // Blink background at 0.75s intervals per spec
            unsigned long now = millis();
            if (now - m_lastBlinkMs >= BLINK_INTERVAL_MS) {
                m_blinkOn = !m_blinkOn;
                m_lastBlinkMs = now;
                if (m_blinkOn) {
                    setItemBackground(m_connectingIndex, m_connectingBgColor);
                } else {
                    clearItemBackground(m_connectingIndex);
                }
            }
            break;
        }
    }
}
