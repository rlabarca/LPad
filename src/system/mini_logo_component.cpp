/**
 * @file mini_logo_component.cpp
 * @brief Mini Logo SystemComponent Implementation
 */

#include "mini_logo_component.h"
#include "../ui_mini_logo.h"

MiniLogoComponent::MiniLogoComponent()
    : m_miniLogo(nullptr)
{
}

MiniLogoComponent::~MiniLogoComponent() {
    delete m_miniLogo;
}

bool MiniLogoComponent::begin(RelativeDisplay* display) {
    if (display == nullptr) return false;
    m_miniLogo = new MiniLogo(display, MiniLogo::Corner::TOP_RIGHT);
    return true;
}

void MiniLogoComponent::render() {
    if (m_miniLogo != nullptr) {
        m_miniLogo->render();
    }
}

bool MiniLogoComponent::handleInput(const touch_gesture_event_t& event) {
    (void)event;
    return false; // Pass-through
}
