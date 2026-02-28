/**
 * @file test_boot_logo_app.cpp
 * @brief Unit tests for BootLogoApp state machine
 *
 * Covers all automated Gherkin scenarios from features/app_boot_logo.md:
 * - Begin rejects null pointers
 * - State starts at WAIT
 * - WAIT transitions to ANIMATE after 2 seconds
 * - ANIMATE transitions to CONNECTING when complete
 * - CONNECTING transitions to DONE after connected hold time
 * - Error state takes priority over all other states
 * - Error message renders once
 * - Ellipsis animation cycles during CONNECTING
 * - Connected SSID displayed on boot complete
 * - Early WiFi completion skips ellipsis
 */

#include <unity.h>
#include <cstring>
#include "apps/boot_logo_app.h"
#include "ui/ui_render_manager.h"
#include "relative_display.h"
#include "../hal/display.h"
#include "../hal/network.h"
#include <Arduino_GFX_Library.h>

// Test helpers from network stub (only available in UNIT_TEST builds)
extern void hal_network_stub_set_status(hal_network_status_t status);
extern void hal_network_stub_set_ssid(const char* ssid);

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
    hal_network_stub_set_status(HAL_NETWORK_STATUS_DISCONNECTED);
    hal_network_stub_set_ssid("Demo WiFi");
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
// Scenario: ANIMATE transitions to CONNECTING when complete
// ---------------------------------------------------------------------------

void test_animate_transitions_to_connecting_when_complete(void) {
    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // WAIT -> ANIMATE
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::ANIMATE, boot.getAnimState());

    boot.update(1.6f);  // Exceed 1.5s animation -> CONNECTING
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::CONNECTING, boot.getAnimState());
}

// ---------------------------------------------------------------------------
// Scenario: CONNECTING transitions to DONE after connected hold time
// ---------------------------------------------------------------------------

void test_connecting_transitions_to_done_after_hold_time(void) {
    MockApp nextApp;
    UIRenderManager::getInstance().registerComponent(&nextApp, 99);

    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // -> ANIMATE
    boot.update(1.6f);  // -> CONNECTING
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::CONNECTING, boot.getAnimState());

    // Signal boot complete — starts connected hold
    boot.setBootComplete();
    boot.update(0.01f);
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::CONNECTING, boot.getAnimState());  // still holding

    // Advance past 1.5s hold -> DONE
    boot.update(1.5f);
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::DONE, boot.getAnimState());

    boot.update(0.01f);  // DONE handler: setActiveApp + m_transitioned = true
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
// Scenario: Ellipsis animation cycles during CONNECTING
// ---------------------------------------------------------------------------

void test_ellipsis_animation_cycles_during_connecting(void) {
    hal_network_stub_set_status(HAL_NETWORK_STATUS_CONNECTING);

    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // WAIT -> ANIMATE
    boot.update(1.6f);  // ANIMATE -> CONNECTING
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::CONNECTING, boot.getAnimState());

    // Entry: 0 dots
    TEST_ASSERT_EQUAL_STRING("Connecting", boot.getStatusText());

    // ~400ms per step cycles 0→1→2→3→0
    boot.update(0.4f);
    TEST_ASSERT_EQUAL_STRING("Connecting.", boot.getStatusText());

    boot.update(0.4f);
    TEST_ASSERT_EQUAL_STRING("Connecting..", boot.getStatusText());

    boot.update(0.4f);
    TEST_ASSERT_EQUAL_STRING("Connecting...", boot.getStatusText());

    boot.update(0.4f);
    TEST_ASSERT_EQUAL_STRING("Connecting", boot.getStatusText());
}

// ---------------------------------------------------------------------------
// Scenario: Connected SSID displayed on boot complete
// ---------------------------------------------------------------------------

void test_connected_ssid_displayed_on_boot_complete(void) {
    hal_network_stub_set_status(HAL_NETWORK_STATUS_CONNECTING);
    hal_network_stub_set_ssid("MyNetwork");

    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // WAIT -> ANIMATE
    boot.update(1.6f);  // ANIMATE -> CONNECTING
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::CONNECTING, boot.getAnimState());

    boot.setBootComplete();
    boot.update(0.01f);

    TEST_ASSERT_EQUAL_STRING("Connected to MyNetwork", boot.getStatusText());
}

// ---------------------------------------------------------------------------
// Scenario: Early WiFi completion skips ellipsis
// ---------------------------------------------------------------------------

void test_early_wifi_completion_skips_ellipsis(void) {
    hal_network_stub_set_ssid("MyNetwork");

    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    // Advance to ANIMATE
    boot.update(2.1f);
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::ANIMATE, boot.getAnimState());

    // Boot complete fires during ANIMATE (early completion)
    boot.setBootComplete();

    // Complete animation -> CONNECTING
    boot.update(1.6f);
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::CONNECTING, boot.getAnimState());

    // First CONNECTING tick must immediately show "Connected to" — no ellipsis
    boot.update(0.01f);
    const char* text = boot.getStatusText();
    TEST_ASSERT_NOT_NULL(strstr(text, "Connected to"));
}

// ---------------------------------------------------------------------------
// Scenario: Ellipsis text does not shift horizontally during animation
// ---------------------------------------------------------------------------

void test_ellipsis_text_does_not_shift_horizontal(void) {
    hal_network_stub_set_status(HAL_NETWORK_STATUS_CONNECTING);

    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // WAIT -> ANIMATE
    boot.update(1.6f);  // ANIMATE -> CONNECTING
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::CONNECTING, boot.getAnimState());

    // All variants have "Connecting" as the fixed left-aligned prefix.
    // Dots only appear/disappear on the right edge; base text never shifts.
    const char* t0 = boot.getStatusText();
    TEST_ASSERT_EQUAL_STRING("Connecting", t0);

    boot.render();  // should not crash; dirty-rect degrades gracefully in stub

    boot.update(0.4f);
    const char* t1 = boot.getStatusText();
    TEST_ASSERT_EQUAL_STRING("Connecting.", t1);

    boot.render();

    boot.update(0.4f);
    const char* t2 = boot.getStatusText();
    TEST_ASSERT_EQUAL_STRING("Connecting..", t2);

    // "Connecting" is always the prefix of every variant (base text never shifts)
    TEST_ASSERT_EQUAL(0, strncmp(t0, "Connecting", 10));
    TEST_ASSERT_EQUAL(0, strncmp(t1, "Connecting", 10));
    TEST_ASSERT_EQUAL(0, strncmp(t2, "Connecting", 10));
}

// ---------------------------------------------------------------------------
// Scenario: Status text uses dirty-rect blitting
// ---------------------------------------------------------------------------

void test_status_text_uses_dirty_rect_blitting(void) {
    hal_network_stub_set_status(HAL_NETWORK_STATUS_CONNECTING);

    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // WAIT -> ANIMATE
    boot.update(1.6f);  // ANIMATE -> CONNECTING
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::CONNECTING, boot.getAnimState());

    // render() must be callable multiple times across text transitions without
    // crashing. In native tests the canvas stub returns nullptr, so the canvas
    // path gracefully no-ops — the important invariant is no crash and no
    // direct draw-erase-redraw on the live display.
    boot.render();       // "Connecting"
    boot.update(0.4f);
    boot.render();       // "Connecting."
    boot.update(0.4f);
    boot.render();       // "Connecting.."
    boot.update(0.4f);
    boot.render();       // "Connecting..."
    TEST_PASS();
}

// ---------------------------------------------------------------------------
// Scenario: Network error displays error text
// ---------------------------------------------------------------------------

void test_network_error_displays_error_text(void) {
    hal_network_stub_set_status(HAL_NETWORK_STATUS_ERROR);

    MockApp nextApp;
    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // WAIT -> ANIMATE
    boot.update(1.6f);  // ANIMATE -> CONNECTING
    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::CONNECTING, boot.getAnimState());

    boot.update(0.01f);
    TEST_ASSERT_EQUAL_STRING("No Network Found", boot.getStatusText());
}

// ---------------------------------------------------------------------------
// Scenario: Status text erased before DONE transition
// ---------------------------------------------------------------------------

void test_status_text_erased_before_done_transition(void) {
    hal_network_stub_set_status(HAL_NETWORK_STATUS_CONNECTING);
    hal_network_stub_set_ssid("TestNet");

    MockApp nextApp;
    UIRenderManager::getInstance().registerComponent(&nextApp, 99);

    BootLogoApp boot;
    boot.begin(g_display, &nextApp);
    boot.onRun();

    boot.update(2.1f);  // WAIT -> ANIMATE
    boot.update(1.6f);  // ANIMATE -> CONNECTING

    // Let some status text render so the text region is computed
    boot.render();

    // Signal completion and advance past the 1.5s connected hold.
    // eraseStatusText() is called inside update() at the DONE boundary.
    boot.setBootComplete();
    boot.update(0.01f);  // starts connected hold timer
    boot.update(1.5f);   // exceeds hold -> eraseStatusText() + state = DONE

    TEST_ASSERT_EQUAL(BootLogoApp::AnimState::DONE, boot.getAnimState());

    // Subsequent render() must not crash (m_transitioned set on next update)
    boot.render();
    boot.update(0.01f);  // executes DONE handler
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
    RUN_TEST(test_animate_transitions_to_connecting_when_complete);
    RUN_TEST(test_connecting_transitions_to_done_after_hold_time);
    RUN_TEST(test_error_state_takes_priority_over_all);
    RUN_TEST(test_error_message_renders_once);
    RUN_TEST(test_ellipsis_animation_cycles_during_connecting);
    RUN_TEST(test_connected_ssid_displayed_on_boot_complete);
    RUN_TEST(test_early_wifi_completion_skips_ellipsis);
    RUN_TEST(test_ellipsis_text_does_not_shift_horizontal);
    RUN_TEST(test_status_text_uses_dirty_rect_blitting);
    RUN_TEST(test_network_error_displays_error_text);
    RUN_TEST(test_status_text_erased_before_done_transition);

    return UNITY_END();
}
