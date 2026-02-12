/**
 * @file ui_render_manager.cpp
 * @brief UIRenderManager implementation
 *
 * Implements the Painter's Algorithm render loop with occlusion optimization,
 * activation-event routing, and SystemComponent pause/resume lifecycle.
 *
 * Specification: features/core_ui_render_manager.md
 * Architecture:  docs/ARCHITECTURE.md §H
 */

#include "ui_render_manager.h"

// ---------------------------------------------------------------------------
// SystemComponent::systemPause — defined here to break circular header dep
// ---------------------------------------------------------------------------
void SystemComponent::systemPause() {
    if (m_manager) {
        m_manager->onSystemComponentPaused(this);
    }
}

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
UIRenderManager& UIRenderManager::getInstance() {
    static UIRenderManager instance;
    return instance;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------
bool UIRenderManager::registerComponent(UIComponent* component, int zOrder) {
    if (component == nullptr || m_componentCount >= MAX_COMPONENTS) {
        return false;
    }

    // Enforce unique Z-Order
    for (int i = 0; i < m_componentCount; i++) {
        if (m_components[i]->getZOrder() == zOrder) {
            return false;
        }
    }

    component->m_zOrder = zOrder;

    // Wire SystemComponents back to this manager
    if (component->getComponentType() == UIComponent::Type::SYSTEM) {
        static_cast<SystemComponent*>(component)->m_manager = this;
    }

    m_components[m_componentCount++] = component;
    sortByZOrder();

    return true;
}

void UIRenderManager::unregisterComponent(UIComponent* component) {
    for (int i = 0; i < m_componentCount; i++) {
        if (m_components[i] == component) {
            if (component->getComponentType() == UIComponent::Type::SYSTEM) {
                static_cast<SystemComponent*>(component)->m_manager = nullptr;
            }

            // Shift remaining entries
            for (int j = i; j < m_componentCount - 1; j++) {
                m_components[j] = m_components[j + 1];
            }
            m_components[--m_componentCount] = nullptr;

            if (m_activeApp == component) {
                m_activeApp = nullptr;
            }
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// App Management
// ---------------------------------------------------------------------------
void UIRenderManager::setActiveApp(AppComponent* app) {
    if (m_activeApp && m_activeApp != app) {
        m_activeApp->m_paused = true;
        m_activeApp->onPause();
    }
    m_activeApp = app;
    if (m_activeApp) {
        m_activeApp->m_paused = false;
        m_activeApp->onRun();
    }
}

// ---------------------------------------------------------------------------
// Render Loop — Painter's Algorithm with Occlusion
// ---------------------------------------------------------------------------
void UIRenderManager::renderAll() {
    int floor = findOcclusionFloor();

    for (int i = floor; i < m_componentCount; i++) {
        UIComponent* comp = m_components[i];
        if (comp->isVisible() && !comp->isPaused()) {
            comp->render();
        }
    }
}

// ---------------------------------------------------------------------------
// Event Routing
// ---------------------------------------------------------------------------
void UIRenderManager::routeInput(const touch_gesture_event_t& event) {
    // Step 1: Check activation events on SystemComponents
    for (int i = 0; i < m_componentCount; i++) {
        UIComponent* comp = m_components[i];
        if (comp->getComponentType() == UIComponent::Type::SYSTEM) {
            SystemComponent* sys = static_cast<SystemComponent*>(comp);
            if (sys->getActivationType() != TOUCH_NONE &&
                event.type == sys->getActivationType() &&
                event.direction == sys->getActivationDirection()) {

                if (sys->isPaused()) {
                    // Pause the active app
                    if (m_activeApp) {
                        m_activeApp->m_paused = true;
                        m_activeApp->onPause();
                    }
                    // Wake up the system component
                    sys->show();
                }
                return; // Event consumed by activation
            }
        }
    }

    // Step 2: Dispatch to active components, highest Z-Order first
    for (int i = m_componentCount - 1; i >= 0; i--) {
        UIComponent* comp = m_components[i];
        if (!comp->isPaused() && comp->isVisible()) {
            if (comp->handleInput(event)) {
                return; // Event consumed
            }
        }
    }
}

// ---------------------------------------------------------------------------
// SystemComponent yield-back
// ---------------------------------------------------------------------------
void UIRenderManager::onSystemComponentPaused(SystemComponent* component) {
    component->hide();

    // Resume the active app
    if (m_activeApp) {
        m_activeApp->m_paused = false;
        m_activeApp->onUnpause();
    }
}

// ---------------------------------------------------------------------------
// Query helpers
// ---------------------------------------------------------------------------
UIComponent* UIRenderManager::getComponentAt(int index) const {
    if (index >= 0 && index < m_componentCount) {
        return m_components[index];
    }
    return nullptr;
}

void UIRenderManager::reset() {
    // Zero our array without dereferencing — components may already be destroyed
    for (int i = 0; i < MAX_COMPONENTS; i++) {
        m_components[i] = nullptr;
    }
    m_componentCount = 0;
    m_activeApp = nullptr;
}

// ---------------------------------------------------------------------------
// Internal: keep m_components sorted by ascending Z-Order
// ---------------------------------------------------------------------------
void UIRenderManager::sortByZOrder() {
    for (int i = 1; i < m_componentCount; i++) {
        UIComponent* key = m_components[i];
        int j = i - 1;
        while (j >= 0 && m_components[j]->getZOrder() > key->getZOrder()) {
            m_components[j + 1] = m_components[j];
            j--;
        }
        m_components[j + 1] = key;
    }
}

// ---------------------------------------------------------------------------
// Internal: find the lowest index from which to start rendering
// ---------------------------------------------------------------------------
int UIRenderManager::findOcclusionFloor() const {
    // Walk from highest Z downward; the first visible, opaque, fullscreen
    // component occludes everything below it.
    for (int i = m_componentCount - 1; i >= 0; i--) {
        UIComponent* comp = m_components[i];
        if (comp->isVisible() && !comp->isPaused() &&
            comp->isOpaque() && comp->isFullscreen()) {
            return i;
        }
    }
    return 0;
}
