/**
 * @file system_menu_component.cpp
 * @brief System Menu SystemComponent Implementation
 *
 * Extracted from SystemMenuAdapter in v070_demo_app.cpp.
 */

#include "system_menu_component.h"
#include <Arduino.h>
#include "../ui/ui_system_menu.h"
#include "../../hal/network.h"

SystemMenuComponent::SystemMenuComponent()
    : m_inner(nullptr)
    , m_closing(false)
{
}

SystemMenuComponent::~SystemMenuComponent() {
    delete m_inner;
}

bool SystemMenuComponent::begin(Arduino_GFX* gfx, int32_t width, int32_t height) {
    m_inner = new SystemMenu();
    if (!m_inner->begin(gfx, width, height)) {
        Serial.println("[SystemMenuComponent] Failed to initialize SystemMenu");
        delete m_inner;
        m_inner = nullptr;
        return false;
    }
    return true;
}

void SystemMenuComponent::setVersion(const char* version) {
    if (m_inner) m_inner->setVersion(version);
}

void SystemMenuComponent::setSSID(const char* ssid) {
    if (m_inner) m_inner->setSSID(ssid);
}

void SystemMenuComponent::setBackgroundColor(uint16_t color) {
    if (m_inner) m_inner->setBackgroundColor(color);
}

void SystemMenuComponent::setRevealColor(uint16_t color) {
    if (m_inner) m_inner->setRevealColor(color);
}

void SystemMenuComponent::setVersionFont(const void* font) {
    if (m_inner) m_inner->setVersionFont(font);
}

void SystemMenuComponent::setVersionColor(uint16_t color) {
    if (m_inner) m_inner->setVersionColor(color);
}

void SystemMenuComponent::setSSIDFont(const void* font) {
    if (m_inner) m_inner->setSSIDFont(font);
}

void SystemMenuComponent::setSSIDColor(uint16_t color) {
    if (m_inner) m_inner->setSSIDColor(color);
}

void SystemMenuComponent::onUnpause() {
    if (m_inner) {
        m_inner->open();
        m_inner->setSSID(hal_network_get_ssid());
    }
    m_closing = false;
    Serial.println("[RenderMgr] SystemMenu: ACTIVATED via EDGE_DRAG TOP");
}

void SystemMenuComponent::update(float dt) {
    if (m_inner == nullptr) return;

    m_inner->update(dt);

    // Detect close-animation completion -> yield control back to manager
    if (m_closing && m_inner->getState() == SystemMenu::CLOSED) {
        m_closing = false;
        Serial.println("[RenderMgr] SystemMenu: CLOSED, calling systemPause()");
        systemPause();
    }
}

void SystemMenuComponent::render() {
    if (m_inner) {
        m_inner->render();
    }
}

bool SystemMenuComponent::handleInput(const touch_gesture_event_t& event) {
    // Close gesture: EDGE_DRAG from BOTTOM edge while menu is open
    if (m_inner != nullptr &&
        event.type == TOUCH_EDGE_DRAG &&
        event.direction == TOUCH_DIR_DOWN &&
        m_inner->getState() == SystemMenu::OPEN) {
        m_inner->close();
        m_closing = true;
        Serial.println("[RenderMgr] SystemMenu: CLOSING via EDGE_DRAG BOTTOM");
        return true;
    }
    // Consume all other input while menu is visible
    return true;
}
