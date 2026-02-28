/**
 * @file test_gesture_engine.cpp
 * @brief Unit tests for TouchGestureEngine
 *
 * Verifies all automated scenarios from features/input_gesture_engine.md.
 * Screen dimensions 400x400 used for round-number threshold calculations.
 *
 * Thresholds for 400x400:
 *   HOLD_THRESHOLD_MS       = 500ms
 *   MOVEMENT_THRESHOLD_PX   = 5% of max(400,400) = 20px
 *   SWIPE_DISTANCE_PX       = 8% of 400 = 32px (axis-aware)
 *   EDGE_ZONE_PX            = 30% of 400 = 120px
 *   EDGE_SWIPE_DISTANCE_PX  = 30% of 400 = 120px
 */

#include <unity.h>
#include "../../src/input/touch_gesture_engine.h"

static const int16_t W = 400;
static const int16_t H = 400;

// --- Helpers ----------------------------------------------------------------

/** Simulate a no-movement touch (finger down, hold, finger up). */
static bool simulate_press_hold_release(TouchGestureEngine& engine,
                                        int16_t x, int16_t y,
                                        uint32_t hold_ms,
                                        touch_gesture_event_t* out_event) {
    touch_gesture_event_t e;
    bool detected = false;

    // Finger down
    engine.update(x, y, true, 0, &e);
    // Hold for hold_ms
    if (engine.update(x, y, true, hold_ms, &e)) {
        *out_event = e;
        detected = true;
    }
    // Finger up (may also produce a gesture)
    if (engine.update(x, y, false, 0, &e)) {
        *out_event = e;
        detected = true;
    }
    return detected;
}

// --- Scenario: Quick tap produces TAP event ---------------------------------

/**
 * Scenario: Quick tap produces TAP event
 * Given the engine is in IDLE state
 * When a finger touches at (50%, 50%) for 200ms with no movement and lifts
 * Then a TOUCH_TAP event is produced at (50%, 50%)
 */
void test_quick_tap_produces_tap_event(void) {
    TouchGestureEngine engine(W, H);
    touch_gesture_event_t event;

    // Finger down at center
    engine.update(200, 200, true, 0, &event);
    // Hold 200ms (below HOLD_THRESHOLD_MS = 500ms)
    engine.update(200, 200, true, 200, &event);
    // Finger up — should produce TAP
    bool detected = engine.update(200, 200, false, 0, &event);

    TEST_ASSERT_TRUE(detected);
    TEST_ASSERT_EQUAL(TOUCH_TAP, event.type);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.5f, event.x_percent);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 0.5f, event.y_percent);
}

// --- Scenario: Hold for 500ms produces HOLD event ---------------------------

/**
 * Scenario: Hold for 500ms produces HOLD event
 * Given the engine is in IDLE state
 * When a finger touches and remains stationary for 500ms
 * Then a TOUCH_HOLD event is produced
 */
void test_hold_for_500ms_produces_hold_event(void) {
    TouchGestureEngine engine(W, H);
    touch_gesture_event_t event;

    // Finger down
    engine.update(200, 200, true, 0, &event);
    // Advance 500ms (at HOLD_THRESHOLD_MS) — HOLD fires
    bool detected = engine.update(200, 200, true, 500, &event);

    TEST_ASSERT_TRUE(detected);
    TEST_ASSERT_EQUAL(TOUCH_HOLD, event.type);
}

// --- Scenario: Movement during hold produces HOLD_DRAG ----------------------

/**
 * Scenario: Movement during hold produces HOLD_DRAG
 * Given a HOLD event has been emitted
 * When the finger moves more than 5% of screen dimension
 * Then TOUCH_HOLD_DRAG events are produced each frame
 */
void test_movement_during_hold_produces_hold_drag(void) {
    TouchGestureEngine engine(W, H);
    touch_gesture_event_t event;

    // Finger down
    engine.update(200, 200, true, 0, &event);
    // Fire HOLD (500ms stationary)
    engine.update(200, 200, true, 500, &event);
    // Move > 5% (movement threshold = 20px); move 50px
    bool detected = engine.update(250, 200, true, 16, &event);

    TEST_ASSERT_TRUE(detected);
    TEST_ASSERT_EQUAL(TOUCH_HOLD_DRAG, event.type);
}

// --- Scenario: Fast swipe produces SWIPE event ------------------------------

/**
 * Scenario: Fast swipe produces SWIPE event
 * Given a finger touches in the center area
 * When it moves 10% horizontally and lifts in under 500ms
 * Then a TOUCH_SWIPE event is produced with LEFT or RIGHT direction
 */
void test_fast_swipe_produces_swipe_event(void) {
    TouchGestureEngine engine(W, H);
    touch_gesture_event_t event;

    // Finger down at center (not near any edge)
    engine.update(200, 200, true, 0, &event);
    // Move 40px right (10% of 400) — above SWIPE_DISTANCE_PX 8% = 32px
    engine.update(240, 200, true, 100, &event);
    // Lift — SWIPE should be produced
    bool detected = engine.update(240, 200, false, 0, &event);

    TEST_ASSERT_TRUE(detected);
    TEST_ASSERT_EQUAL(TOUCH_SWIPE, event.type);
    TEST_ASSERT_EQUAL(TOUCH_DIR_RIGHT, event.direction);
}

// --- Scenario: Edge drag from top produces EDGE_DRAG UP ---------------------

/**
 * Scenario: Edge drag from top produces EDGE_DRAG UP
 * Given a finger touches within the top 30% of the screen
 * When it moves 30% downward and lifts
 * Then a TOUCH_EDGE_DRAG event is produced with direction TOUCH_DIR_UP
 */
void test_edge_drag_from_top_produces_edge_drag_up(void) {
    TouchGestureEngine engine(W, H);
    touch_gesture_event_t event;

    // Start at y=50 (12.5% of 400 — within top 30% edge zone)
    engine.update(200, 50, true, 0, &event);
    // Move 150px downward (37.5%) — above EDGE_SWIPE_DISTANCE_PX = 120px
    engine.update(200, 200, true, 100, &event);
    // Lift
    bool detected = engine.update(200, 200, false, 0, &event);

    TEST_ASSERT_TRUE(detected);
    TEST_ASSERT_EQUAL(TOUCH_EDGE_DRAG, event.type);
    TEST_ASSERT_EQUAL(TOUCH_DIR_UP, event.direction);
}

// --- Scenario: Small movement on release does not produce swipe -------------

/**
 * Scenario: Small movement on release does not produce swipe
 * Given a finger touches and moves only 5% before lifting
 * When under 500ms has elapsed
 * Then a TOUCH_TAP event is produced (not a SWIPE)
 */
void test_small_movement_on_release_produces_tap_not_swipe(void) {
    TouchGestureEngine engine(W, H);
    touch_gesture_event_t event;

    // Finger down at center
    engine.update(200, 200, true, 0, &event);
    // Move only 15px (3.75% of 400 — below MOVEMENT_THRESHOLD_PERCENT 5% = 20px)
    engine.update(215, 200, true, 100, &event);
    // Lift in under 500ms
    bool detected = engine.update(215, 200, false, 0, &event);

    TEST_ASSERT_TRUE(detected);
    TEST_ASSERT_EQUAL(TOUCH_TAP, event.type);
}

// --- Scenario: Axis-aware thresholds use correct dimension ------------------

/**
 * Scenario: Axis-aware thresholds use correct dimension
 * Given the screen is 536 wide and 240 tall
 * When a horizontal swipe of 8% of 536 pixels occurs
 * Then SWIPE is detected using the width-based threshold
 */
void test_axis_aware_thresholds_use_correct_dimension(void) {
    TouchGestureEngine engine(536, 240);
    touch_gesture_event_t event;

    // 8% of 536 = 42.88px, round up to 43px — just above threshold
    engine.update(268, 120, true, 0, &event);
    engine.update(311, 120, true, 100, &event);  // moved 43px right
    bool detected = engine.update(311, 120, false, 0, &event);

    TEST_ASSERT_TRUE(detected);
    TEST_ASSERT_EQUAL(TOUCH_SWIPE, event.type);
}

// --- Scenario: Custom edge zones override defaults --------------------------

/**
 * Scenario: Custom edge zones override defaults
 * Given setEdgeZones(40, 430, 36, 204) is called
 * When a touch starts at x=30 (within left edge zone)
 * Then it is classified as starting from the left edge
 */
void test_custom_edge_zones_override_defaults(void) {
    TouchGestureEngine engine(W, H);
    engine.setEdgeZones(40, 430, 36, 204);

    touch_gesture_event_t event;

    // Touch at x=30 (inside left edge zone threshold of 40)
    engine.update(30, 100, true, 0, &event);
    // Move right 220px (55%) — well above EDGE_SWIPE_DISTANCE_PX
    engine.update(250, 100, true, 100, &event);
    // Lift
    bool detected = engine.update(250, 100, false, 0, &event);

    TEST_ASSERT_TRUE(detected);
    TEST_ASSERT_EQUAL(TOUCH_EDGE_DRAG, event.type);
    TEST_ASSERT_EQUAL(TOUCH_DIR_LEFT, event.direction);
}

// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_quick_tap_produces_tap_event);
    RUN_TEST(test_hold_for_500ms_produces_hold_event);
    RUN_TEST(test_movement_during_hold_produces_hold_drag);
    RUN_TEST(test_fast_swipe_produces_swipe_event);
    RUN_TEST(test_edge_drag_from_top_produces_edge_drag_up);
    RUN_TEST(test_small_movement_on_release_produces_tap_not_swipe);
    RUN_TEST(test_axis_aware_thresholds_use_correct_dimension);
    RUN_TEST(test_custom_edge_zones_override_defaults);

    return UNITY_END();
}
