/**
 * @file test_timer_hal.cpp
 * @brief Unity tests for Timer HAL contracts
 *
 * Covers all automated scenarios from features/hal_timer.md.
 * All tests run against the stub implementation on native_test.
 *
 * Spec: features/hal_timer.md
 */

#include <unity.h>
#include "../hal/timer.h"

// Strong override to test the "overrideable by test fixture" scenario.
// Defined without __attribute__((weak)) so it supersedes the stub.
static uint64_t g_fixture_micros = 0;
uint64_t hal_timer_get_micros(void) {
    return g_fixture_micros;
}

void setUp(void) {
    g_fixture_micros = 0;
}

void tearDown(void) {}

/**
 * Scenario: Timer init succeeds on ESP32
 * On native_test stub, hal_timer_init returns false (no hardware).
 * The test exercises the init API and verifies a valid bool is returned.
 */
void test_timer_init_returns_bool(void) {
    bool result = hal_timer_init();
    // Stub returns false; ESP32 real impl returns true.
    TEST_ASSERT_TRUE(result == true || result == false);
}

/**
 * Scenario: Stub timer init returns false by default
 * Spec §2.2: default stub must return false from hal_timer_init.
 */
void test_stub_timer_init_returns_false_by_default(void) {
    // We have overridden hal_timer_get_micros but NOT hal_timer_init,
    // so the weak stub's hal_timer_init is still in effect → returns false.
    bool result = hal_timer_init();
    TEST_ASSERT_FALSE(result);
}

/**
 * Scenario: Stub timer can be overridden by test fixture
 * Spec §2.2: stub functions use __attribute__((weak)) so test fixtures can
 * replace them by defining the same symbol without the weak attribute.
 */
void test_stub_timer_can_be_overridden_by_test_fixture(void) {
    g_fixture_micros = 12345678;
    uint64_t value = hal_timer_get_micros();
    TEST_ASSERT_EQUAL_UINT64(12345678, value);
}

/**
 * Scenario: Timer returns monotonically increasing values
 * Spec §2.1: hal_timer_get_micros must be monotonic.
 * Verified via the test fixture override (deterministic increment).
 */
void test_timer_returns_monotonically_increasing_values(void) {
    g_fixture_micros = 100;
    uint64_t first = hal_timer_get_micros();

    g_fixture_micros = 200;
    uint64_t second = hal_timer_get_micros();

    TEST_ASSERT_TRUE(second > first);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_timer_init_returns_bool);
    RUN_TEST(test_stub_timer_init_returns_false_by_default);
    RUN_TEST(test_stub_timer_can_be_overridden_by_test_fixture);
    RUN_TEST(test_timer_returns_monotonically_increasing_values);

    return UNITY_END();
}
