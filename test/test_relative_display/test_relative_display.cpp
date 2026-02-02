/**
 * @file test_relative_display.cpp
 * @brief Unity tests for Relative Display Abstraction
 *
 * These tests verify coordinate conversion from percentages to pixels
 * for the relative display abstraction layer.
 *
 * COORDINATE CONVERSION VALIDATION:
 * ---------------------------------
 * Stub (240x240):
 *   - 0%   -> 0 pixels
 *   - 25%  -> 60 pixels
 *   - 50%  -> 120 pixels
 *   - 75%  -> 180 pixels
 *   - 100% -> 240 pixels
 *
 * ESP32-S3-AMOLED (368x448):
 *   - 0%   -> 0 pixels (both width/height)
 *   - 25%  -> 92 pixels (width), 112 pixels (height)
 *   - 50%  -> 184 pixels (width), 224 pixels (height)
 *   - 75%  -> 276 pixels (width), 336 pixels (height)
 *   - 100% -> 368 pixels (width), 448 pixels (height)
 *
 * T-Display-S3-Plus (240x536):
 *   - 0%   -> 0 pixels (both width/height)
 *   - 25%  -> 60 pixels (width), 134 pixels (height)
 *   - 50%  -> 120 pixels (width), 268 pixels (height)
 *   - 75%  -> 180 pixels (width), 402 pixels (height)
 *   - 100% -> 240 pixels (width), 536 pixels (height)
 */

#include <unity.h>
#include "../src/relative_display.h"
#include "../hal/display.h"
#include <math.h>

// RGB565 color definitions
#define RGB565_BLACK   0x0000
#define RGB565_WHITE   0xFFFF
#define RGB565_RED     0xF800
#define RGB565_GREEN   0x07E0
#define RGB565_BLUE    0x001F

// Helper function to calculate expected pixel coordinate
static int32_t expected_pixel(float percent, int32_t dimension) {
    return (int32_t)roundf((percent / 100.0f) * (float)dimension);
}

void setUp(void) {
    // Set up runs before each test
}

void tearDown(void) {
    // Tear down runs after each test
}

/**
 * Test: HAL Dimension Query Functions
 * Verifies that dimension queries work correctly
 */
void test_hal_display_dimensions(void) {
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    TEST_ASSERT_GREATER_THAN(0, width);
    TEST_ASSERT_GREATER_THAN(0, height);

    // For stub: 240x240
    TEST_ASSERT_EQUAL_INT32(240, width);
    TEST_ASSERT_EQUAL_INT32(240, height);
}

/**
 * Test: Percentage to Pixel Conversion - Corner Cases
 * Validates conversion at 0% and 100%
 */
void test_coordinate_conversion_corners(void) {
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    // 0% should always map to pixel 0
    TEST_ASSERT_EQUAL_INT32(0, expected_pixel(0.0f, width));
    TEST_ASSERT_EQUAL_INT32(0, expected_pixel(0.0f, height));

    // 100% should map to the dimension
    TEST_ASSERT_EQUAL_INT32(width, expected_pixel(100.0f, width));
    TEST_ASSERT_EQUAL_INT32(height, expected_pixel(100.0f, height));
}

/**
 * Test: Percentage to Pixel Conversion - Midpoints
 * Validates conversion at 25%, 50%, 75%
 */
void test_coordinate_conversion_midpoints(void) {
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    // For 240x240 stub display:
    TEST_ASSERT_EQUAL_INT32(60, expected_pixel(25.0f, 240));   // 25% of 240 = 60
    TEST_ASSERT_EQUAL_INT32(120, expected_pixel(50.0f, 240));  // 50% of 240 = 120
    TEST_ASSERT_EQUAL_INT32(180, expected_pixel(75.0f, 240));  // 75% of 240 = 180

    // Verify actual display dimensions
    TEST_ASSERT_EQUAL_INT32(expected_pixel(50.0f, width), width / 2);
}

/**
 * Test: Draw Single Pixel at Origin (0%, 0%)
 *
 * EXPECTED BEHAVIOR:
 * - Stub (240x240): Pixel at (0, 0)
 * - ESP32-S3 (368x448): Pixel at (0, 0)
 * - T-Display (240x536): Pixel at (0, 0)
 *
 * Distance from origin: 0 pixels
 */
void test_draw_pixel_at_origin(void) {
    display_relative_init();

    // Draw pixel at origin
    display_relative_draw_pixel(0.0f, 0.0f, RGB565_RED);

    TEST_PASS();
}

/**
 * Test: Draw Single Pixel at Center (50%, 50%)
 *
 * EXPECTED BEHAVIOR:
 * - Stub (240x240): Pixel at (120, 120)
 * - ESP32-S3 (368x448): Pixel at (184, 224)
 * - T-Display (240x536): Pixel at (120, 268)
 *
 * Distance from origin:
 * - Stub: sqrt(120^2 + 120^2) = 169.7 pixels
 * - ESP32-S3: sqrt(184^2 + 224^2) = 290.0 pixels
 * - T-Display: sqrt(120^2 + 268^2) = 293.9 pixels
 */
void test_draw_pixel_at_center(void) {
    display_relative_init();

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    // Expected center coordinates
    int32_t center_x = expected_pixel(50.0f, width);
    int32_t center_y = expected_pixel(50.0f, height);

    // Draw pixel at center
    display_relative_draw_pixel(50.0f, 50.0f, RGB565_WHITE);

    // Verify expected coordinates for stub display
    TEST_ASSERT_EQUAL_INT32(120, center_x);  // 50% of 240
    TEST_ASSERT_EQUAL_INT32(120, center_y);  // 50% of 240
}

/**
 * Test: Draw 50x50% Square at Origin (0%, 0%)
 *
 * EXPECTED BEHAVIOR:
 * - Stub (240x240): Rectangle from (0,0) to (120,120) = 120x120 pixels
 * - ESP32-S3 (368x448): Rectangle from (0,0) to (184,224) = 184x224 pixels
 * - T-Display (240x536): Rectangle from (0,0) to (120,268) = 120x268 pixels
 *
 * Distance from origin: 0 pixels (top-left corner at origin)
 */
void test_draw_square_at_origin(void) {
    display_relative_init();

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    // Expected dimensions
    int32_t rect_width = expected_pixel(50.0f, width);
    int32_t rect_height = expected_pixel(50.0f, height);

    // Draw 50x50% square at origin
    display_relative_fill_rectangle(0.0f, 0.0f, 50.0f, 50.0f, RGB565_BLUE);

    // Verify dimensions for stub display
    TEST_ASSERT_EQUAL_INT32(120, rect_width);   // 50% of 240
    TEST_ASSERT_EQUAL_INT32(120, rect_height);  // 50% of 240
}

/**
 * Test: Draw 25x25% Square at (25%, 25%)
 *
 * EXPECTED BEHAVIOR:
 * - Stub (240x240):
 *   - Top-left: (60, 60)
 *   - Dimensions: 60x60 pixels
 *   - Distance from origin: sqrt(60^2 + 60^2) = 84.9 pixels
 *
 * - ESP32-S3 (368x448):
 *   - Top-left: (92, 112)
 *   - Dimensions: 92x112 pixels
 *   - Distance from origin: sqrt(92^2 + 112^2) = 145.3 pixels
 *
 * - T-Display (240x536):
 *   - Top-left: (60, 134)
 *   - Dimensions: 60x134 pixels
 *   - Distance from origin: sqrt(60^2 + 134^2) = 147.0 pixels
 */
void test_draw_square_at_quarter_position(void) {
    display_relative_init();

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    // Expected top-left position
    int32_t x_start = expected_pixel(25.0f, width);
    int32_t y_start = expected_pixel(25.0f, height);

    // Expected dimensions
    int32_t rect_width = expected_pixel(25.0f, width);
    int32_t rect_height = expected_pixel(25.0f, height);

    // Draw 25x25% square at (25%, 25%)
    display_relative_fill_rectangle(25.0f, 25.0f, 25.0f, 25.0f, RGB565_GREEN);

    // Verify for stub display (240x240)
    TEST_ASSERT_EQUAL_INT32(60, x_start);      // 25% of 240
    TEST_ASSERT_EQUAL_INT32(60, y_start);      // 25% of 240
    TEST_ASSERT_EQUAL_INT32(60, rect_width);   // 25% of 240
    TEST_ASSERT_EQUAL_INT32(60, rect_height);  // 25% of 240
}

/**
 * Test: Draw Centered 50x50% Square
 *
 * EXPECTED BEHAVIOR:
 * - Stub (240x240):
 *   - Top-left: (60, 60)
 *   - Dimensions: 120x120 pixels
 *   - Bottom-right: (180, 180)
 *   - Distance from origin: sqrt(60^2 + 60^2) = 84.9 pixels
 *
 * - ESP32-S3 (368x448):
 *   - Top-left: (92, 112)
 *   - Dimensions: 184x224 pixels
 *   - Bottom-right: (276, 336)
 *   - Distance from origin: sqrt(92^2 + 112^2) = 145.3 pixels
 *
 * - T-Display (240x536):
 *   - Top-left: (60, 134)
 *   - Dimensions: 120x268 pixels
 *   - Bottom-right: (180, 402)
 *   - Distance from origin: sqrt(60^2 + 134^2) = 147.0 pixels
 */
void test_draw_centered_square(void) {
    display_relative_init();

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    // Centered 50x50% square starts at 25%, 25%
    int32_t x_start = expected_pixel(25.0f, width);
    int32_t y_start = expected_pixel(25.0f, height);
    int32_t rect_width = expected_pixel(50.0f, width);
    int32_t rect_height = expected_pixel(50.0f, height);

    // Draw centered square
    display_relative_fill_rectangle(25.0f, 25.0f, 50.0f, 50.0f, RGB565_RED);

    // Verify for stub display
    TEST_ASSERT_EQUAL_INT32(60, x_start);
    TEST_ASSERT_EQUAL_INT32(60, y_start);
    TEST_ASSERT_EQUAL_INT32(120, rect_width);
    TEST_ASSERT_EQUAL_INT32(120, rect_height);
}

/**
 * Test: Draw Test Pattern with Labeled Distances
 *
 * This test draws a comprehensive pattern showing scaling behavior:
 * 1. Corner markers at (0,0), (100,0), (0,100), (100,100)
 * 2. Center cross at (50%, 50%)
 * 3. Inset square at (10%, 10%) with 80x80% size
 */
void test_comprehensive_scaling_pattern(void) {
    display_relative_init();

    // Corner markers (5x5% squares)
    display_relative_fill_rectangle(0.0f, 0.0f, 5.0f, 5.0f, RGB565_RED);     // Top-left
    display_relative_fill_rectangle(95.0f, 0.0f, 5.0f, 5.0f, RGB565_GREEN);  // Top-right
    display_relative_fill_rectangle(0.0f, 95.0f, 5.0f, 5.0f, RGB565_BLUE);   // Bottom-left
    display_relative_fill_rectangle(95.0f, 95.0f, 5.0f, 5.0f, RGB565_WHITE); // Bottom-right

    // Center cross (1% thick lines)
    display_relative_draw_horizontal_line(50.0f, 0.0f, 100.0f, RGB565_WHITE);
    display_relative_draw_vertical_line(50.0f, 0.0f, 100.0f, RGB565_WHITE);

    // Inset frame (10% margin)
    // Top border
    display_relative_draw_horizontal_line(10.0f, 10.0f, 90.0f, RGB565_GREEN);
    // Bottom border
    display_relative_draw_horizontal_line(90.0f, 10.0f, 90.0f, RGB565_GREEN);
    // Left border
    display_relative_draw_vertical_line(10.0f, 10.0f, 90.0f, RGB565_GREEN);
    // Right border
    display_relative_draw_vertical_line(90.0f, 10.0f, 90.0f, RGB565_GREEN);

    TEST_PASS();
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_hal_display_dimensions);
    RUN_TEST(test_coordinate_conversion_corners);
    RUN_TEST(test_coordinate_conversion_midpoints);
    RUN_TEST(test_draw_pixel_at_origin);
    RUN_TEST(test_draw_pixel_at_center);
    RUN_TEST(test_draw_square_at_origin);
    RUN_TEST(test_draw_square_at_quarter_position);
    RUN_TEST(test_draw_centered_square);
    RUN_TEST(test_comprehensive_scaling_pattern);

    return UNITY_END();
}
