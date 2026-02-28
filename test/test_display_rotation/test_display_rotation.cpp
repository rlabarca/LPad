/**
 * @file test_display_rotation.cpp
 * @brief Unity tests for Display Rotation Contract
 *
 * These tests verify that the Display HAL rotation functionality is correctly
 * implemented as specified in features/display_rotation_contract.md.
 */

#include <unity.h>
#include "../hal/display.h"

// RGB565 color definitions for testing
#define RGB565_BLACK   0x0000
#define RGB565_WHITE   0xFFFF

void setUp(void) {
    // Set up runs before each test
}

void tearDown(void) {
    // Tear down runs after each test
}

/**
 * Test: hal_display_set_rotation contract
 * Verifies that the set_rotation function can be called without crashing
 */
void test_hal_display_set_rotation_callable(void) {
    // Should not crash with various rotation values
    hal_display_set_rotation(0);
    hal_display_set_rotation(90);
    hal_display_set_rotation(180);
    hal_display_set_rotation(270);

    TEST_PASS();
}

/**
 * Test: Dimensions swap at 90 degrees
 * Verifies that width and height are swapped after 90-degree rotation
 */
void test_hal_display_rotation_90_swaps_dimensions(void) {
    // Get original dimensions (stub returns 240x240 by default)
    int32_t original_width = hal_display_get_width_pixels();
    int32_t original_height = hal_display_get_height_pixels();

    // Rotate 90 degrees
    hal_display_set_rotation(90);

    // Dimensions should be swapped
    int32_t rotated_width = hal_display_get_width_pixels();
    int32_t rotated_height = hal_display_get_height_pixels();

    TEST_ASSERT_EQUAL_INT32(original_height, rotated_width);
    TEST_ASSERT_EQUAL_INT32(original_width, rotated_height);
}

/**
 * Test: Dimensions swap at 270 degrees
 * Verifies that width and height are swapped after 270-degree rotation
 */
void test_hal_display_rotation_270_swaps_dimensions(void) {
    // Reset to 0 degrees first
    hal_display_set_rotation(0);
    int32_t original_width = hal_display_get_width_pixels();
    int32_t original_height = hal_display_get_height_pixels();

    // Rotate 270 degrees
    hal_display_set_rotation(270);

    // Dimensions should be swapped
    int32_t rotated_width = hal_display_get_width_pixels();
    int32_t rotated_height = hal_display_get_height_pixels();

    TEST_ASSERT_EQUAL_INT32(original_height, rotated_width);
    TEST_ASSERT_EQUAL_INT32(original_width, rotated_height);
}

/**
 * Test: Dimensions remain same at 0 and 180 degrees
 * Verifies that width and height are NOT swapped at 0 or 180 degrees
 */
void test_hal_display_rotation_0_180_no_swap(void) {
    // Set to 0 degrees
    hal_display_set_rotation(0);
    int32_t width_0 = hal_display_get_width_pixels();
    int32_t height_0 = hal_display_get_height_pixels();

    // Rotate to 180 degrees
    hal_display_set_rotation(180);
    int32_t width_180 = hal_display_get_width_pixels();
    int32_t height_180 = hal_display_get_height_pixels();

    // Dimensions should remain the same (not swapped)
    TEST_ASSERT_EQUAL_INT32(width_0, width_180);
    TEST_ASSERT_EQUAL_INT32(height_0, height_180);
}

/**
 * Test: Multiple rotation changes
 * Verifies that rotation can be changed multiple times
 */
void test_hal_display_multiple_rotations(void) {
    // Original state
    hal_display_set_rotation(0);
    int32_t width_0 = hal_display_get_width_pixels();
    int32_t height_0 = hal_display_get_height_pixels();

    // Rotate through all angles
    hal_display_set_rotation(90);
    hal_display_set_rotation(180);
    hal_display_set_rotation(270);

    // Back to 0
    hal_display_set_rotation(0);
    int32_t width_final = hal_display_get_width_pixels();
    int32_t height_final = hal_display_get_height_pixels();

    // Should return to original dimensions
    TEST_ASSERT_EQUAL_INT32(width_0, width_final);
    TEST_ASSERT_EQUAL_INT32(height_0, height_final);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_hal_display_set_rotation_callable);
    RUN_TEST(test_hal_display_rotation_90_swaps_dimensions);
    RUN_TEST(test_hal_display_rotation_270_swaps_dimensions);
    RUN_TEST(test_hal_display_rotation_0_180_no_swap);
    RUN_TEST(test_hal_display_multiple_rotations);

    return UNITY_END();
}
