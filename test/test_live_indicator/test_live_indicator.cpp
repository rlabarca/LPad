/**
 * @file test_live_indicator.cpp
 * @brief Unity tests for LiveIndicator component
 *
 * These tests verify the animated live indicator component with pulsing animation.
 */

#include <unity.h>
#include "../../src/ui_live_indicator.h"
#include "../../src/relative_display.h"
#include <cmath>

// RGB565 color definitions
#define RGB565_RED     0xF800
#define RGB565_BLUE    0x001F
#define RGB565_PINK    0xF81F
#define RGB565_CYAN    0x07FF

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Mock Arduino_GFX class for testing
class MockArduinoGFX : public Arduino_GFX {
public:
    MockArduinoGFX(int16_t w, int16_t h) : Arduino_GFX(w, h) {}

    bool begin(int32_t speed = 0) override { return true; }
    void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {}
    void drawPixel(int16_t x, int16_t y, uint16_t color) override {}
};

// Global test fixtures
MockArduinoGFX* mockGfx = nullptr;
RelativeDisplay* relDisplay = nullptr;
LiveIndicator* indicator = nullptr;

void setUp(void) {
    // Create a 200x200 mock display for testing
    mockGfx = new MockArduinoGFX(200, 200);
    relDisplay = new RelativeDisplay(mockGfx, 200, 200);

    // Initialize the procedural API for gradient drawing
    display_relative_init();
}

void tearDown(void) {
    delete indicator;
    delete relDisplay;
    delete mockGfx;
    indicator = nullptr;
    relDisplay = nullptr;
    mockGfx = nullptr;
}

/**
 * Test: Indicator initialization
 * Verifies that the indicator is created with correct initial state
 */
void test_indicator_initialization(void) {
    IndicatorTheme theme;
    theme.innerColor = RGB565_RED;
    theme.outerColor = RGB565_BLUE;
    theme.minRadius = 2.0f;
    theme.maxRadius = 10.0f;
    theme.pulseDuration = 1000.0f;  // 1 second

    indicator = new LiveIndicator(theme, relDisplay);

    // Initial radius at phase = 0: sin(0) = 0, t = 0.5, pulse_factor = 0.5
    // radius = 2 + (10-2)*0.5 = 6
    float initial_radius = indicator->getCurrentRadius();
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 6.0f, initial_radius);
}

/**
 * Test: Pulse animation progression
 * Scenario: Animating the Indicator
 * Given a LiveIndicator is initialized with a theme (minRadius=2, maxRadius=10)
 * When update(deltaTime) is called repeatedly over the pulseDuration
 * Then the current radius should oscillate smoothly between min and max
 */
void test_pulse_animation(void) {
    IndicatorTheme theme;
    theme.innerColor = RGB565_PINK;
    theme.outerColor = RGB565_CYAN;
    theme.minRadius = 2.0f;
    theme.maxRadius = 10.0f;
    theme.pulseDuration = 1000.0f;  // 1 second for full cycle

    indicator = new LiveIndicator(theme, relDisplay);

    // Update through quarter cycle (250ms = 0.25 seconds)
    // At 0.25 seconds, phase = PI/2, sin(PI/2) = 1, t = 1, pulse_factor = 1, radius = maxRadius
    indicator->update(0.25f);
    float radius_quarter = indicator->getCurrentRadius();
    TEST_ASSERT_FLOAT_WITHIN(1.0f, theme.maxRadius, radius_quarter);

    // Update through half cycle (another 250ms, total 500ms = 0.5 seconds)
    // At 0.5 seconds, phase = PI, sin(PI) = 0, t = 0.5, pulse_factor = 0.5, radius = 6 (middle)
    indicator->update(0.25f);
    float radius_half = indicator->getCurrentRadius();
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 6.0f, radius_half);

    // Update through three-quarter cycle (another 250ms, total 750ms = 0.75 seconds)
    // At 0.75 seconds, phase = 3*PI/2, sin(3*PI/2) = -1, t = 0, pulse_factor = 0, radius = minRadius
    indicator->update(0.25f);
    float radius_three_quarter = indicator->getCurrentRadius();
    TEST_ASSERT_FLOAT_WITHIN(1.0f, theme.minRadius, radius_three_quarter);

    // Update through full cycle (another 250ms, total 1000ms = 1.0 second)
    // At 1.0 seconds, phase = 2*PI, sin(2*PI) = 0, t = 0.5, radius = 6 (middle, back to start)
    indicator->update(0.25f);
    float radius_full = indicator->getCurrentRadius();
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 6.0f, radius_full);
}

/**
 * Test: Reset functionality
 * Verifies that reset() returns the animation to the starting state
 */
void test_reset(void) {
    IndicatorTheme theme;
    theme.innerColor = RGB565_RED;
    theme.outerColor = RGB565_BLUE;
    theme.minRadius = 2.0f;
    theme.maxRadius = 10.0f;
    theme.pulseDuration = 1000.0f;

    indicator = new LiveIndicator(theme, relDisplay);

    // Advance animation to phase = PI (radius = 6)
    indicator->update(0.5f);
    float radius_before = indicator->getCurrentRadius();

    // Reset should return to initial state (phase = 0, radius = 6)
    indicator->reset();
    float radius_after = indicator->getCurrentRadius();

    TEST_ASSERT_FLOAT_WITHIN(0.5f, 6.0f, radius_after);
    // In this case, they happen to be the same since both are at middle position
    // Let's test with a different update time
    indicator->update(0.25f);  // Move to phase = PI/2 (radius = 10)
    float radius_moved = indicator->getCurrentRadius();
    TEST_ASSERT_FLOAT_WITHIN(1.0f, theme.maxRadius, radius_moved);

    // Reset and verify we're back to initial
    indicator->reset();
    float radius_reset = indicator->getCurrentRadius();
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 6.0f, radius_reset);
}

/**
 * Test: Drawing at specified position
 * Scenario: Rendering a Static Indicator
 * Given the RelativeDisplay is initialized
 * When the LiveIndicator is drawn at position (50, 50) with a fixed radius
 * Then the draw() method should complete without error
 */
void test_draw_at_position(void) {
    IndicatorTheme theme;
    theme.innerColor = RGB565_RED;
    theme.outerColor = RGB565_BLUE;
    theme.minRadius = 5.0f;
    theme.maxRadius = 5.0f;  // Fixed radius for this test
    theme.pulseDuration = 1000.0f;

    indicator = new LiveIndicator(theme, relDisplay);

    // Drawing should not crash or throw
    indicator->draw(50.0f, 50.0f);

    // Verify radius is as expected
    float radius = indicator->getCurrentRadius();
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 5.0f, radius);
}

/**
 * Test: Zero pulse duration (static indicator)
 * Verifies that an indicator with zero pulse duration doesn't animate
 */
void test_zero_pulse_duration(void) {
    IndicatorTheme theme;
    theme.innerColor = RGB565_PINK;
    theme.outerColor = RGB565_CYAN;
    theme.minRadius = 5.0f;
    theme.maxRadius = 10.0f;
    theme.pulseDuration = 0.0f;  // No animation

    indicator = new LiveIndicator(theme, relDisplay);

    float initial_radius = indicator->getCurrentRadius();

    // Update should have no effect
    indicator->update(0.5f);
    float radius_after = indicator->getCurrentRadius();

    TEST_ASSERT_EQUAL_FLOAT(initial_radius, radius_after);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_indicator_initialization);
    RUN_TEST(test_pulse_animation);
    RUN_TEST(test_reset);
    RUN_TEST(test_draw_at_position);
    RUN_TEST(test_zero_pulse_duration);

    return UNITY_END();
}
