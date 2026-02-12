/**
 * @file test_render_manager.cpp
 * @brief Unit tests for UIRenderManager, UIComponent, AppComponent, SystemComponent
 *
 * Covers all Gherkin scenarios from features/core_ui_render_manager.md:
 * - Registration and Z-Order enforcement
 * - Render order (Painter's Algorithm)
 * - Occlusion optimization
 * - App switching (Pause/Resume via activation events)
 * - System Menu closing (systemPause)
 * - Event routing (highest Z first, propagation stop)
 */

#include <unity.h>
#include "ui/ui_render_manager.h"

// ==========================================
// Render-order tracking
// ==========================================
static int g_renderOrder[16];
static int g_renderCount = 0;

static void resetTracking() {
    g_renderCount = 0;
    for (int i = 0; i < 16; i++) g_renderOrder[i] = -1;
}

// ==========================================
// Mock Components
// ==========================================

class MockApp : public AppComponent {
public:
    int id;
    bool opaqueFlag = false;
    bool fullscreenFlag = false;
    bool consumeInput = false;
    int lastInputType = -1;
    int pauseCalls = 0;
    int unpauseCalls = 0;
    int runCalls = 0;
    int closeCalls = 0;

    MockApp(int id) : id(id) {}

    void onRun() override { runCalls++; }
    void onPause() override { pauseCalls++; }
    void onUnpause() override { unpauseCalls++; }
    void onClose() override { closeCalls++; }

    void render() override {
        if (g_renderCount < 16) g_renderOrder[g_renderCount++] = id;
    }

    bool handleInput(const touch_gesture_event_t& event) override {
        lastInputType = static_cast<int>(event.type);
        return consumeInput;
    }

    bool isOpaque() const override { return opaqueFlag; }
    bool isFullscreen() const override { return fullscreenFlag; }
};

class MockSystem : public SystemComponent {
public:
    int id;
    bool opaqueFlag = false;
    bool fullscreenFlag = false;
    bool consumeInput = false;
    int lastInputType = -1;
    int pauseCalls = 0;
    int unpauseCalls = 0;

    MockSystem(int id) : id(id) {}

    void onPause() override { pauseCalls++; }
    void onUnpause() override { unpauseCalls++; }

    void render() override {
        if (g_renderCount < 16) g_renderOrder[g_renderCount++] = id;
    }

    bool handleInput(const touch_gesture_event_t& event) override {
        lastInputType = static_cast<int>(event.type);
        return consumeInput;
    }

    bool isOpaque() const override { return opaqueFlag; }
    bool isFullscreen() const override { return fullscreenFlag; }
};

// ==========================================
// Setup & Teardown
// ==========================================

void setUp(void) {
    UIRenderManager::getInstance().reset();
    resetTracking();
}

void tearDown(void) {}

// ==========================================
// Scenario: Registration and Z-Order Enforcement
// ==========================================

void test_register_components_succeed() {
    MockApp bg(0);
    MockApp ticker(1);

    auto& mgr = UIRenderManager::getInstance();
    TEST_ASSERT_TRUE(mgr.registerComponent(&bg, 0));
    TEST_ASSERT_TRUE(mgr.registerComponent(&ticker, 1));
    TEST_ASSERT_EQUAL(2, mgr.getComponentCount());
}

void test_duplicate_zorder_fails() {
    MockApp ticker(1);
    MockSystem status(2);

    auto& mgr = UIRenderManager::getInstance();
    TEST_ASSERT_TRUE(mgr.registerComponent(&ticker, 1));
    TEST_ASSERT_FALSE(mgr.registerComponent(&status, 1));
    TEST_ASSERT_EQUAL(1, mgr.getComponentCount());
}

void test_null_registration_fails() {
    TEST_ASSERT_FALSE(UIRenderManager::getInstance().registerComponent(nullptr, 0));
}

void test_components_sorted_by_zorder() {
    MockSystem menu(20);
    MockApp ticker(1);
    MockSystem mini(10);

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&menu, 20);
    mgr.registerComponent(&ticker, 1);
    mgr.registerComponent(&mini, 10);

    TEST_ASSERT_EQUAL(1, mgr.getComponentAt(0)->getZOrder());
    TEST_ASSERT_EQUAL(10, mgr.getComponentAt(1)->getZOrder());
    TEST_ASSERT_EQUAL(20, mgr.getComponentAt(2)->getZOrder());
}

// ==========================================
// Scenario: Rendering Order and Occlusion
// ==========================================

void test_render_ascending_z_order() {
    MockApp ticker(1);
    MockSystem mini(10);
    MockSystem menu(20);

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&ticker, 1);
    mgr.registerComponent(&mini, 10);
    mgr.registerComponent(&menu, 20);

    mgr.renderAll();

    TEST_ASSERT_EQUAL(3, g_renderCount);
    TEST_ASSERT_EQUAL(1, g_renderOrder[0]);
    TEST_ASSERT_EQUAL(10, g_renderOrder[1]);
    TEST_ASSERT_EQUAL(20, g_renderOrder[2]);
}

void test_occlusion_by_opaque_fullscreen() {
    // Scenario: SystemMenu (Z=20) is opaque + fullscreen → occlude lower
    MockApp ticker(1);
    MockSystem mini(10);
    MockSystem menu(20);

    menu.opaqueFlag = true;
    menu.fullscreenFlag = true;

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&ticker, 1);
    mgr.registerComponent(&mini, 10);
    mgr.registerComponent(&menu, 20);

    mgr.renderAll();

    TEST_ASSERT_EQUAL(1, g_renderCount);
    TEST_ASSERT_EQUAL(20, g_renderOrder[0]);
}

void test_transparent_overlay_no_occlusion() {
    // Scenario: MiniLogo (Z=10) is NOT opaque → both render
    MockApp ticker(1);
    MockSystem mini(10);

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&ticker, 1);
    mgr.registerComponent(&mini, 10);

    mgr.renderAll();

    TEST_ASSERT_EQUAL(2, g_renderCount);
    TEST_ASSERT_EQUAL(1, g_renderOrder[0]);
    TEST_ASSERT_EQUAL(10, g_renderOrder[1]);
}

void test_paused_hidden_component_not_rendered() {
    MockApp ticker(1);
    MockSystem menu(20);

    menu.hide(); // Paused + hidden

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&ticker, 1);
    mgr.registerComponent(&menu, 20);

    mgr.renderAll();

    TEST_ASSERT_EQUAL(1, g_renderCount);
    TEST_ASSERT_EQUAL(1, g_renderOrder[0]);
}

// ==========================================
// Scenario: App Switching (Pause/Resume)
// ==========================================

void test_activation_event_pauses_app_wakes_system() {
    MockApp ticker(1);
    MockSystem menu(20);

    menu.setActivationEvent(TOUCH_EDGE_DRAG, TOUCH_DIR_UP);
    menu.hide(); // Start hidden

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&ticker, 1);
    mgr.registerComponent(&menu, 20);
    mgr.setActiveApp(&ticker);

    // Clear setup counters
    ticker.pauseCalls = 0;
    ticker.runCalls = 0;
    menu.unpauseCalls = 0;

    // Fire activation gesture
    touch_gesture_event_t event = {};
    event.type = TOUCH_EDGE_DRAG;
    event.direction = TOUCH_DIR_UP;
    mgr.routeInput(event);

    // Ticker should be paused
    TEST_ASSERT_TRUE(ticker.isPaused());
    TEST_ASSERT_EQUAL(1, ticker.pauseCalls);

    // Menu should be visible and unpaused
    TEST_ASSERT_TRUE(menu.isVisible());
    TEST_ASSERT_FALSE(menu.isPaused());
    TEST_ASSERT_EQUAL(1, menu.unpauseCalls);
}

// ==========================================
// Scenario: System Menu Closing
// ==========================================

void test_system_pause_hides_menu_resumes_app() {
    MockApp ticker(1);
    MockSystem menu(20);

    menu.setActivationEvent(TOUCH_EDGE_DRAG, TOUCH_DIR_UP);
    menu.hide();

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&ticker, 1);
    mgr.registerComponent(&menu, 20);
    mgr.setActiveApp(&ticker);

    // Activate menu
    touch_gesture_event_t event = {};
    event.type = TOUCH_EDGE_DRAG;
    event.direction = TOUCH_DIR_UP;
    mgr.routeInput(event);

    // Clear counters
    ticker.unpauseCalls = 0;
    menu.pauseCalls = 0;

    // Menu calls systemPause (user closed it)
    menu.systemPause();

    // Menu should be hidden/paused
    TEST_ASSERT_FALSE(menu.isVisible());
    TEST_ASSERT_TRUE(menu.isPaused());
    TEST_ASSERT_EQUAL(1, menu.pauseCalls);

    // Ticker should be resumed
    TEST_ASSERT_FALSE(ticker.isPaused());
    TEST_ASSERT_EQUAL(1, ticker.unpauseCalls);
}

// ==========================================
// Scenario: Event Routing
// ==========================================

void test_input_dispatched_highest_z_first() {
    MockApp app(1);
    MockSystem overlay(10);

    app.consumeInput = true;
    overlay.consumeInput = true;

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&app, 1);
    mgr.registerComponent(&overlay, 10);

    touch_gesture_event_t event = {};
    event.type = TOUCH_TAP;
    mgr.routeInput(event);

    // Overlay (Z=10) gets it first and consumes
    TEST_ASSERT_EQUAL(TOUCH_TAP, overlay.lastInputType);
    // App should NOT receive (overlay consumed)
    TEST_ASSERT_EQUAL(-1, app.lastInputType);
}

void test_input_falls_through_when_not_consumed() {
    MockApp app(1);
    MockSystem overlay(10);

    app.consumeInput = true;
    overlay.consumeInput = false; // Does not consume

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&app, 1);
    mgr.registerComponent(&overlay, 10);

    touch_gesture_event_t event = {};
    event.type = TOUCH_TAP;
    mgr.routeInput(event);

    TEST_ASSERT_EQUAL(TOUCH_TAP, overlay.lastInputType);
    TEST_ASSERT_EQUAL(TOUCH_TAP, app.lastInputType);
}

void test_paused_component_skipped_for_input() {
    MockApp app(1);
    MockSystem sys(10);

    sys.consumeInput = true;
    sys.hide(); // Paused
    app.consumeInput = true;

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&app, 1);
    mgr.registerComponent(&sys, 10);

    touch_gesture_event_t event = {};
    event.type = TOUCH_TAP;
    mgr.routeInput(event);

    TEST_ASSERT_EQUAL(-1, sys.lastInputType);
    TEST_ASSERT_EQUAL(TOUCH_TAP, app.lastInputType);
}

void test_activation_event_consumed_no_dispatch() {
    MockApp app(1);
    MockSystem menu(20);

    menu.setActivationEvent(TOUCH_EDGE_DRAG, TOUCH_DIR_UP);
    menu.hide();
    app.consumeInput = true;

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&app, 1);
    mgr.registerComponent(&menu, 20);
    mgr.setActiveApp(&app);

    touch_gesture_event_t event = {};
    event.type = TOUCH_EDGE_DRAG;
    event.direction = TOUCH_DIR_UP;
    mgr.routeInput(event);

    // Activation consumed the event — app should NOT see it
    TEST_ASSERT_EQUAL(-1, app.lastInputType);
}

// ==========================================
// Test: App management
// ==========================================

void test_set_active_app_calls_on_run() {
    MockApp app(1);

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&app, 1);
    mgr.setActiveApp(&app);

    TEST_ASSERT_EQUAL(1, app.runCalls);
    TEST_ASSERT_FALSE(app.isPaused());
}

void test_switching_app_pauses_previous() {
    MockApp app1(1);
    MockApp app2(2);

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&app1, 1);
    mgr.registerComponent(&app2, 5);

    mgr.setActiveApp(&app1);
    TEST_ASSERT_EQUAL(1, app1.runCalls);

    mgr.setActiveApp(&app2);
    TEST_ASSERT_EQUAL(1, app1.pauseCalls);
    TEST_ASSERT_EQUAL(1, app2.runCalls);
}

// ==========================================
// Test: Unregister
// ==========================================

void test_unregister_removes_component() {
    MockApp app(1);
    MockSystem sys(10);

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&app, 1);
    mgr.registerComponent(&sys, 10);
    TEST_ASSERT_EQUAL(2, mgr.getComponentCount());

    mgr.unregisterComponent(&sys);
    TEST_ASSERT_EQUAL(1, mgr.getComponentCount());
}

void test_unregister_active_app_clears_pointer() {
    MockApp app(1);

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&app, 1);
    mgr.setActiveApp(&app);
    TEST_ASSERT_NOT_NULL(mgr.getActiveApp());

    mgr.unregisterComponent(&app);
    TEST_ASSERT_NULL(mgr.getActiveApp());
}

void test_unregister_allows_zorder_reuse() {
    MockApp app1(1);
    MockApp app2(2);

    auto& mgr = UIRenderManager::getInstance();
    mgr.registerComponent(&app1, 5);
    mgr.unregisterComponent(&app1);

    // Z-Order 5 should now be available
    TEST_ASSERT_TRUE(mgr.registerComponent(&app2, 5));
}

// ==========================================
// Main
// ==========================================

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // Registration & Z-Order
    RUN_TEST(test_register_components_succeed);
    RUN_TEST(test_duplicate_zorder_fails);
    RUN_TEST(test_null_registration_fails);
    RUN_TEST(test_components_sorted_by_zorder);

    // Rendering
    RUN_TEST(test_render_ascending_z_order);
    RUN_TEST(test_occlusion_by_opaque_fullscreen);
    RUN_TEST(test_transparent_overlay_no_occlusion);
    RUN_TEST(test_paused_hidden_component_not_rendered);

    // App management
    RUN_TEST(test_set_active_app_calls_on_run);
    RUN_TEST(test_switching_app_pauses_previous);

    // App switching via activation
    RUN_TEST(test_activation_event_pauses_app_wakes_system);

    // System Menu closing
    RUN_TEST(test_system_pause_hides_menu_resumes_app);

    // Event routing
    RUN_TEST(test_input_dispatched_highest_z_first);
    RUN_TEST(test_input_falls_through_when_not_consumed);
    RUN_TEST(test_paused_component_skipped_for_input);
    RUN_TEST(test_activation_event_consumed_no_dispatch);

    // Unregister
    RUN_TEST(test_unregister_removes_component);
    RUN_TEST(test_unregister_active_app_clears_pointer);
    RUN_TEST(test_unregister_allows_zorder_reuse);

    return UNITY_END();
}
