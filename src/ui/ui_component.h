/**
 * @file ui_component.h
 * @brief Abstract base classes for the UI component hierarchy
 *
 * Defines UIComponent (base), AppComponent (full-screen apps), and
 * SystemComponent (persistent overlays) used by the UIRenderManager.
 *
 * Specification: features/core_ui_render_manager.md
 * Architecture:  docs/ARCHITECTURE.md §H
 */

#ifndef UI_COMPONENT_H
#define UI_COMPONENT_H

#include <stdint.h>
#include "../input/touch_gesture_engine.h"

class UIRenderManager;

/**
 * @brief Abstract base class for all renderable/interactive UI elements.
 *
 * Components are registered with the UIRenderManager at a specific Z-Order.
 * The manager calls lifecycle methods (onRun, onPause, onUnpause) and
 * render/handleInput each frame based on visibility and occlusion state.
 */
class UIComponent {
public:
    enum class Type { APP, SYSTEM };

    virtual ~UIComponent() = default;

    virtual Type getComponentType() const = 0;

    // Lifecycle
    virtual void onRun() {}
    virtual void onPause() {}
    virtual void onUnpause() {}
    virtual void render() = 0;
    virtual bool handleInput(const touch_gesture_event_t& event) { return false; }

    // Properties for render manager occlusion check
    virtual bool isOpaque() const { return false; }
    virtual bool isFullscreen() const { return false; }

    bool isVisible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    bool isPaused() const { return m_paused; }
    int getZOrder() const { return m_zOrder; }

protected:
    bool m_visible = true;
    bool m_paused = false;
    int m_zOrder = 0;

    friend class UIRenderManager;
};

/**
 * @brief A full-screen application component. Only one can be active at a time.
 */
class AppComponent : public UIComponent {
public:
    Type getComponentType() const override { return Type::APP; }
    virtual void onClose() {}
};

/**
 * @brief A persistent system overlay (e.g., System Menu, Status Bar, Mini Logo).
 *
 * Multiple SystemComponents can run simultaneously. Each can register an
 * activation event (gesture) that wakes it from a paused/hidden state.
 */
class SystemComponent : public UIComponent {
public:
    Type getComponentType() const override { return Type::SYSTEM; }

    void show() {
        m_visible = true;
        m_paused = false;
        onUnpause();
    }

    void hide() {
        m_visible = false;
        m_paused = true;
        onPause();
    }

    /** Yields control back to the UIRenderManager (implemented in ui_render_manager.cpp). */
    void systemPause();

    void setActivationEvent(touch_gesture_type_t type, touch_direction_t dir) {
        m_activationType = type;
        m_activationDirection = dir;
    }

    touch_gesture_type_t getActivationType() const { return m_activationType; }
    touch_direction_t getActivationDirection() const { return m_activationDirection; }

private:
    touch_gesture_type_t m_activationType = TOUCH_NONE;
    touch_direction_t m_activationDirection = TOUCH_DIR_NONE;
    UIRenderManager* m_manager = nullptr;

    friend class UIRenderManager;
};

#endif // UI_COMPONENT_H
