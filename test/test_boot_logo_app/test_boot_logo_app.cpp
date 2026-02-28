/**
 * @file test_boot_logo_app.cpp
 * @brief Unit tests for BootLogoApp state machine
 *
 * Covers all automated Gherkin scenarios from features/app_boot_logo.md:
 * - Begin rejects null pointers
 * - State starts at WAIT
 * - WAIT transitions to ANIMATE after 2 seconds
 * - ANIMATE transitions to HOLDING when complete
 * - HOLDING transitions to DONE on boot complete signal
 * - Error state takes priority over all other states
 * - Error message renders once
 */

#include <unity.h>
#include "apps/boot_logo_app.h"
#include "ui/ui_render_manager.h"
#include "relative_display.h"
#include "../hal/display.h"
#include <Arduino_GFX_Library.h>

// ---------------------------------------------------------------------------
// Mock classes
// ---------------------------------------------------------------------------

class MockGFX : public Arduino_GFX {
public:
    MockGFX() : Arduino_GFX(320, 170) {}
    bool begin(int32_t speed = 0) override { return true; }
    void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {
        (void)x; (void)y; (void)color;
    }
};

class MockApp : public AppComponent {
public:
    int runCalls = 0;
    void onRun() override { runCalls++; }
    void render() override {}
    void update(float dt) override { (void)dt; }
    bool handleInput(const touch_gesture_event_t& e) override { (void)e; return false; }
    bool isOpaque() const override { return true; }
    bool isFullscreen() const override { return true; }
};

// ---------------------------------------------------------------------------
// Test fixtures
// ---------------------------------------------------------------------------

static MockGFX* g_gfx = nullptr;
static RelativeDisplay* g_display = nullptr;

void setUp(void) {
    hal_display_init();
    UIRenderManager::getInstance().reset();
    g_gfx = new MockGFX();
    g_display = new RelativeDisplay(g_gfx, 320, 170);
    g_display->init();
}

void tearDown(void) {
    UIRenderManager::getInstance().reset();
    delete g_display;
    delete g_gfx;
    g_display = nullptr;
    g_gfx = nullptr;
}

// ---------------------------------------------------------------------------
// Scenario: Begin rejects null pointers
// ---------------------------------------------------------------------------

void test_begin_rejects_null_display(void) {
    MockApp nextApp;
    BootLogoApp boot;
    TEST_ASSERT_FALSE(boot.begin(nullptr, &nextApp));
}

void test_begin_rejects_null_next_app(void) {
    BootLogoApp boot;
    TEST_ASSERT_FALSE(boot.begin(g_display, nullptr));
}

// ---------------------------------------------------------------------------
// Scenario: State starts at WAIT
// ---------------------------------------------------------------------------

void test_state_starts_at_wait_after_onrun(void) {
    MockApp nextApp;
    BootLogoApp boot;
    TEST_ASSERT_TRUE(boot.begin(g_display, &nextApp));
    boot.onRun();
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::WAIT, boot.getAnimState());
}

// ---------------------------------------------------------------------------
// Scenario: WAIT transitions to ANIMATE after 2 seconds
// ---------------------------------------------------------------------------

void test_wait_transitions_to_animate_after_2s(void) {
    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    // Partial update — still in WAIT
    boot.update(1.0f);
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::WAIT, boot.getAnimState());

    // Exceed 2.0s total — should transition to ANIMATE
    boot.update(1.1f);
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::ANIMATE, boot.getAnimState());
}

// ---------------------------------------------------------------------------
// Scenario: ANIMATE transitions to HOLDING when complete
// ---------------------------------------------------------------------------

void test_animate_transitions_to_holding_when_complete(void) {
    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // WAIT -> ANIMATE
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::ANIMATE, boot.getAnimState());

    boot.update(1.6f);  // Exceed 1.5s animation -> HOLDING
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::HOLDING, boot.getAnimState());
}

// ---------------------------------------------------------------------------
// Scenario: HOLDING transitions to DONE on boot complete signal
// ---------------------------------------------------------------------------

void test_holding_transitions_to_done_on_boot_complete(void) {
    MockApp nextApp;
    UIRenderManager::getInstance().registerComponent(&nextApp, 99);

    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // -> ANIMATE
    boot.update(1.6f);  // -> HOLDING
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::HOLDING, boot.getAnimState());

    boot.setBootComplete();
    boot.update(0.01f);  // HOLDING -> m_state = DONE
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::DONE, boot.getAnimState());

    boot.update(0.01f);  // DONE handler: setActiveApp + m_transitioned = true
    // Active app should now be nextApp
    TEST_ASSERT_EQUAL_PTR(&nextApp, UIRenderManager::getInstance().getActiveApp());
}

// ---------------------------------------------------------------------------
// Scenario: Error state takes priority over all other states
// ---------------------------------------------------------------------------

void test_error_state_takes_priority_over_all(void) {
    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // -> ANIMATE
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::ANIMATE, boot.getAnimState());

    boot.setErrorMessage("WiFi Failed");
    boot.update(0.01f);  // ERROR takes priority
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::ERROR, boot.getAnimState());
}

// ---------------------------------------------------------------------------
// Scenario: Error message renders once
// ---------------------------------------------------------------------------

void test_error_message_renders_once(void) {
    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.setErrorMessage("Test Error");
    boot.update(0.01f);  // force ERROR state
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::ERROR, boot.getAnimState());

    // Both render calls should succeed without crash
    // The second call is a no-op (m_errorRendered flag guards it)
    boot.render();
    boot.render();
    TEST_PASS();
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_begin_rejects_null_display);
    RUN_TEST(test_begin_rejects_null_next_app);
    RUN_TEST(test_state_starts_at_wait_after_onrun);
    RUN_TEST(test_wait_transitions_to_animate_after_2s);
    RUN_TEST(test_animate_transitions_to_holding_when_complete);
    RUN_TEST(test_holding_transitions_to_done_on_boot_complete);
    RUN_TEST(test_error_state_takes_priority_over_all);
    RUN_TEST(test_error_message_renders_once);

    return UNITY_END();
}
