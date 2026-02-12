/**
 * @file ui_render_manager.h
 * @brief Central singleton managing component lifecycle, rendering, and input routing
 *
 * The UIRenderManager replaces the monolithic demo-app pattern with a composable
 * system where Apps and SystemComponents are registered, Z-ordered, and managed.
 *
 * Specification: features/core_ui_render_manager.md
 * Architecture:  docs/ARCHITECTURE.md §H
 */

#ifndef UI_RENDER_MANAGER_H
#define UI_RENDER_MANAGER_H

#include "ui_component.h"

class UIRenderManager {
public:
    static UIRenderManager& getInstance();

    /**
     * Register a component at the given Z-Order.
     * @return false if zOrder is already taken, component is null, or registry is full.
     */
    bool registerComponent(UIComponent* component, int zOrder);
    void unregisterComponent(UIComponent* component);

    /** Set the active application. Pauses any previous app, calls onRun() on the new one. */
    void setActiveApp(AppComponent* app);
    AppComponent* getActiveApp() const { return m_activeApp; }

    /** Render all visible, non-paused components in ascending Z-Order (Painter's Algorithm). */
    void renderAll();

    /** Route a touch event: first checks activation events, then dispatches highest-Z first. */
    void routeInput(const touch_gesture_event_t& event);

    /** Called by SystemComponent::systemPause() to yield control back to the manager. */
    void onSystemComponentPaused(SystemComponent* component);

    int getComponentCount() const { return m_componentCount; }
    UIComponent* getComponentAt(int index) const;

    /** Clear all registrations (for testing). */
    void reset();

    static constexpr int MAX_COMPONENTS = 16;

private:
    UIRenderManager() = default;
    UIRenderManager(const UIRenderManager&) = delete;
    UIRenderManager& operator=(const UIRenderManager&) = delete;

    UIComponent* m_components[MAX_COMPONENTS] = {};
    int m_componentCount = 0;
    AppComponent* m_activeApp = nullptr;

    void sortByZOrder();
    int findOcclusionFloor() const;
};

#endif // UI_RENDER_MANAGER_H
