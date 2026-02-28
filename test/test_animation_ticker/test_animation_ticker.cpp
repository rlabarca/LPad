/**
 * @file test_animation_ticker.cpp
 * @brief Unity tests for AnimationTicker
 *
 * These tests verify the AnimationTicker behavior as specified in
 * features/app_animation_ticker.md.
 */

#include <unity.h>
#include "../../src/animation_ticker.h"

// Mock state for hal_timer_get_micros
static uint64_t mock_current_time_micros = 0;

// Mock state to track delay calls
static uint64_t total_delay_micros = 0;

// Mock implementation of hal_timer_get_micros
extern "C" uint64_t hal_timer_get_micros(void) {
    return mock_current_time_micros;
}

// Mock implementation of hal_timer_init
extern "C" bool hal_timer_init(void) {
    return true;
}

// Mock implementation of delay (Arduino function)
extern "C" void delay(unsigned long ms) {
    total_delay_micros += ms * 1000;
}

// Mock implementation of delayMicroseconds (Arduino function)
extern "C" void delayMicroseconds(unsigned int us) {
    total_delay_micros += us;
}

void setUp(void) {
    // Reset mock state before each test
    mock_current_time_micros = 0;
    total_delay_micros = 0;
}

void tearDown(void) {
    // Cleanup after each test
}

/**
 * Test Case 1: Verify that waitForNextFrame introduces a delay when the
 * "work" in the frame is shorter than the frame time.
 */
void test_wait_introduces_delay_when_work_is_fast(void) {
    // Create a 30fps ticker (frame time = 33333 microseconds)
    AnimationTicker ticker(30);

    // First call should return immediately and not introduce delay
    mock_current_time_micros = 1000000;  // 1 second
    ticker.waitForNextFrame();
    TEST_ASSERT_EQUAL(0, total_delay_micros);

    // Simulate fast work: only 10ms (10000 microseconds) of work
    mock_current_time_micros += 10000;
    total_delay_micros = 0;

    // Second call should introduce delay to reach the frame time
    ticker.waitForNextFrame();

    // Expected delay should be approximately (33333 - 10000) = 23333 microseconds
    // Allow some tolerance for rounding in the implementation
    TEST_ASSERT_UINT_WITHIN(100, 23333, total_delay_micros);
}

/**
 * Test Case 2: Verify that waitForNextFrame does not introduce a delay
 * when the "work" in the frame is longer than the frame time.
 */
void test_no_delay_when_work_exceeds_frame_time(void) {
    // Create a 30fps ticker (frame time = 33333 microseconds)
    AnimationTicker ticker(30);

    // First call should return immediately
    mock_current_time_micros = 1000000;  // 1 second
    ticker.waitForNextFrame();

    // Simulate slow work: 50ms (50000 microseconds), which exceeds frame time
    mock_current_time_micros += 50000;
    total_delay_micros = 0;

    // Second call should NOT introduce any delay
    ticker.waitForNextFrame();

    TEST_ASSERT_EQUAL(0, total_delay_micros);
}

/**
 * Test Case 3: Verify the "death spiral" guard correctly resets the
 * next_frame_time when the ticker falls behind significantly.
 */
void test_death_spiral_guard_resets_schedule(void) {
    // Create a 30fps ticker (frame time = 33333 microseconds)
    AnimationTicker ticker(30);

    // First call at time T0
    mock_current_time_micros = 1000000;  // 1 second
    ticker.waitForNextFrame();

    // Simulate very slow work that misses multiple frames
    // Next frame was scheduled at 1033333, but we're now at 1200000
    mock_current_time_micros = 1200000;
    total_delay_micros = 0;

    // This call should trigger the death spiral guard and not try to catch up
    ticker.waitForNextFrame();
    TEST_ASSERT_EQUAL(0, total_delay_micros);

    // Now do fast work (10ms) and verify we're back on track with the NEW schedule
    mock_current_time_micros += 10000;  // Now at 1210000
    total_delay_micros = 0;

    ticker.waitForNextFrame();

    // Should have scheduled next frame at 1200000 + 33333 = 1233333
    // Current time is 1210000, so should wait 23333 microseconds
    TEST_ASSERT_UINT_WITHIN(100, 23333, total_delay_micros);
}

/**
 * Test: First call to waitForNextFrame should not introduce any delay
 */
void test_first_call_no_delay(void) {
    AnimationTicker ticker(30);

    mock_current_time_micros = 5000000;
    total_delay_micros = 0;

    ticker.waitForNextFrame();

    TEST_ASSERT_EQUAL(0, total_delay_micros);
}

/**
 * Test: Verify frame rate timing is correct for 30fps
 */
void test_frame_rate_30fps(void) {
    AnimationTicker ticker(30);

    // First call
    mock_current_time_micros = 0;
    ticker.waitForNextFrame();

    // 30fps = 1,000,000 / 30 = 33333.33... microseconds per frame
    // Expected frame time is 33333 microseconds

    // Do minimal work and check delay
    mock_current_time_micros = 1000;  // 1ms of work
    total_delay_micros = 0;

    ticker.waitForNextFrame();

    // Should wait approximately 32333 microseconds (33333 - 1000)
    TEST_ASSERT_UINT_WITHIN(100, 32333, total_delay_micros);
}

/**
 * Test: Verify waitForNextFrame returns correct deltaTime values
 */
void test_returns_correct_delta_time(void) {
    AnimationTicker ticker(30);

    // First call should return 0.0f
    mock_current_time_micros = 1000000;  // 1 second
    float deltaTime = ticker.waitForNextFrame();
    TEST_ASSERT_EQUAL_FLOAT(0.0f, deltaTime);

    // Second call after 10ms should return 0.01 seconds
    mock_current_time_micros += 10000;  // +10ms
    deltaTime = ticker.waitForNextFrame();
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.01f, deltaTime);

    // Third call after 50ms should return 0.05 seconds
    mock_current_time_micros += 50000;  // +50ms
    deltaTime = ticker.waitForNextFrame();
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.05f, deltaTime);

    // Fourth call after exactly one frame (33333 microseconds)
    mock_current_time_micros += 33333;
    deltaTime = ticker.waitForNextFrame();
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.033333f, deltaTime);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_first_call_no_delay);
    RUN_TEST(test_wait_introduces_delay_when_work_is_fast);
    RUN_TEST(test_no_delay_when_work_exceeds_frame_time);
    RUN_TEST(test_death_spiral_guard_resets_schedule);
    RUN_TEST(test_frame_rate_30fps);
    RUN_TEST(test_returns_correct_delta_time);

    return UNITY_END();
}
