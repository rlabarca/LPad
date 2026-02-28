/**
 * @file test_wifi_list.cpp
 * @brief Unit tests for WiFiListWidget
 *
 * Tests connection lifecycle, automatic fallback, and blink animation.
 *
 * Specification: features/widget_wifi_list.md
 */

#include <unity.h>
#include "ui/widgets/wifi_list_widget.h"
#include "../hal/network.h"

// HAL stub test helpers (defined in hal/network_stub.cpp under UNIT_TEST)
#ifdef UNIT_TEST
extern void hal_network_stub_set_status(hal_network_status_t status);
#endif

// Millis test helper (defined in wifi_list_widget.cpp under UNIT_TEST)
#ifdef UNIT_TEST
extern void wifi_list_widget_set_fake_millis(unsigned long ms);
#endif

// ============================================================================
// Test entries
// ============================================================================

static const WiFiListWidget::WiFiEntry g_entries[] = {
    {"NetworkA", "passA"},
    {"NetworkB", "passB"}
};

// ============================================================================
// Setup & Teardown
// ============================================================================

void setUp(void) {
#ifdef UNIT_TEST
    hal_network_stub_set_status(HAL_NETWORK_STATUS_DISCONNECTED);
    wifi_list_widget_set_fake_millis(0);
#endif
}

void tearDown(void) {}

// ============================================================================
// Scenario: Tap initiates connection attempt
// ============================================================================

void test_tap_initiates_connection_attempt() {
    WiFiListWidget widget;
    widget.setEntries(g_entries, 2);

    // Verify no network is connecting before the tap
    TEST_ASSERT_EQUAL_INT(-1, widget.getConnectingIndex());

    // Tap first network (lineHeight=20 default, y=5 → item 0)
    touch_gesture_event_t tap = {};
    tap.type = TOUCH_TAP;
    tap.x_px = 50;
    tap.y_px = 5;
    widget.handleInput(tap, 0, 0, 100, 200);

    // Entry must be in Connecting state: connectingIndex set and blink background ON
    TEST_ASSERT_EQUAL_INT(0, widget.getConnectingIndex());
    TEST_ASSERT_TRUE(widget.getItemHasBg(0));
}

// ============================================================================
// Scenario: Automatic fallback to last good network
// ============================================================================

void test_automatic_fallback_to_last_good_network() {
    WiFiListWidget widget;
    widget.setEntries(g_entries, 2);

    // Step 1: Connect to NetworkA and finalize (sets lastGoodIndex=0)
    touch_gesture_event_t tapA = {};
    tapA.type = TOUCH_TAP;
    tapA.x_px = 50;
    tapA.y_px = 5; // Item 0 = NetworkA
    widget.handleInput(tapA, 0, 0, 100, 200);
    // Stub is CONNECTED after init; update() finalizes the connection
    widget.update();
    TEST_ASSERT_EQUAL_INT(0, widget.getActiveIndex());

    // Step 2: Attempt to connect to NetworkB
    hal_network_stub_set_status(HAL_NETWORK_STATUS_DISCONNECTED);
    touch_gesture_event_t tapB = {};
    tapB.type = TOUCH_TAP;
    tapB.x_px = 50;
    tapB.y_px = 25; // Item 1 = NetworkB (y=25 → item 1 with lineHeight=20)
    widget.handleInput(tapB, 0, 0, 100, 200);
    TEST_ASSERT_EQUAL_INT(1, widget.getConnectingIndex());

    // Step 3: Simulate NetworkB failure → fallback to NetworkA
    hal_network_stub_set_status(HAL_NETWORK_STATUS_ERROR);
    widget.update();

    // After B fails, automatic reconnect to A (lastGoodIndex=0) must be initiated
    TEST_ASSERT_EQUAL_INT(0, widget.getConnectingIndex());
}

// ============================================================================
// Scenario: Blink toggles every 750ms during connecting
// ============================================================================

void test_blink_toggles_every_750ms_during_connecting() {
    WiFiListWidget widget;
    widget.setEntries(g_entries, 2);

    // Trigger connection to NetworkA (handleSelection sets blinkOn=true, lastBlinkMs=0)
    touch_gesture_event_t tap = {};
    tap.type = TOUCH_TAP;
    tap.x_px = 50;
    tap.y_px = 5; // Item 0
    widget.handleInput(tap, 0, 0, 100, 200);

    // Initial state: connecting, background ON (blink ON)
    TEST_ASSERT_EQUAL_INT(0, widget.getConnectingIndex());
    TEST_ASSERT_TRUE(widget.getItemHasBg(0));

    // Override HAL to CONNECTING so update() processes blink (not connection success)
    hal_network_stub_set_status(HAL_NETWORK_STATUS_CONNECTING);

    // At t=0ms: no toggle yet (0 < 750)
    widget.update();
    TEST_ASSERT_TRUE(widget.getItemHasBg(0)); // Still ON

    // At t=750ms: toggle fires → blink OFF → background cleared
    wifi_list_widget_set_fake_millis(750);
    widget.update();
    TEST_ASSERT_FALSE(widget.getItemHasBg(0)); // Toggled OFF

    // At t=1500ms: toggle again → blink ON → background restored
    wifi_list_widget_set_fake_millis(1500);
    widget.update();
    TEST_ASSERT_TRUE(widget.getItemHasBg(0)); // Toggled ON
}

// ============================================================================
// Test Runner
// ============================================================================

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_tap_initiates_connection_attempt);
    RUN_TEST(test_automatic_fallback_to_last_good_network);
    RUN_TEST(test_blink_toggles_every_750ms_during_connecting);

    return UNITY_END();
}
