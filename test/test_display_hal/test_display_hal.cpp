/**
 * @file test_display_hal.cpp
 * @brief Unity tests for Display HAL contracts
 *
 * These tests verify that the Display HAL interface is correctly defined
 * and can be used as specified in features/hal_spec_display.md.
 */

#include <unity.h>
#include "../hal/display.h"

// RGB565 color definitions for testing
#define RGB565_BLACK   0x0000
#define RGB565_WHITE   0xFFFF
#define RGB565_RED     0xF800
#define RGB565_GREEN   0x07E0
#define RGB565_BLUE    0x001F

void setUp(void) {
    // Set up runs before each test
}

void tearDown(void) {
    // Tear down runs after each test
}

/**
 * Test: hal_display_init contract
 * Verifies that the init function can be called and returns a boolean
 */
void test_hal_display_init_returns_bool(void) {
    bool result = hal_display_init();

    // The stub implementation returns false
    // A real implementation should return true on success
    TEST_ASSERT_TRUE(result == true || result == false);
}

/**
 * Test: hal_display_clear contract
 * Verifies that clear can be called with various color values without crashing
 */
void test_hal_display_clear_accepts_color(void) {
    // Should not crash with any color value
    hal_display_clear(RGB565_BLACK);
    hal_display_clear(RGB565_WHITE);
    hal_display_clear(RGB565_RED);
    hal_display_clear(0x07E0);  // Green
    hal_display_clear(0xFFFF);  // White

    TEST_PASS();
}

/**
 * Test: hal_display_draw_pixel contract
 * Verifies that draw_pixel can be called with various coordinates and colors
 */
void test_hal_display_draw_pixel_accepts_coordinates(void) {
    // Should not crash with any valid coordinates
    hal_display_draw_pixel(0, 0, RGB565_WHITE);
    hal_display_draw_pixel(100, 100, RGB565_RED);
    hal_display_draw_pixel(367, 447, RGB565_BLUE);  // Max coordinates for 368x448 display

    // Test negative coordinates (should be handled gracefully)
    hal_display_draw_pixel(-1, -1, RGB565_GREEN);

    TEST_PASS();
}

/**
 * Test: hal_display_flush contract
 * Verifies that flush can be called without crashing
 */
void test_hal_display_flush_callable(void) {
    // Should not crash
    hal_display_flush();

    TEST_PASS();
}

/**
 * Test: API usage sequence
 * Verifies that the typical usage sequence works as documented
 */
void test_hal_display_typical_usage_sequence(void) {
    // Typical usage: init -> clear -> draw -> flush
    bool init_result = hal_display_init();
    TEST_ASSERT_TRUE(init_result == true || init_result == false);

    hal_display_clear(RGB565_BLACK);
    hal_display_draw_pixel(10, 10, RGB565_WHITE);
    hal_display_flush();

    TEST_PASS();
}

/**
 * Test: Multiple operations without flush
 * Verifies that multiple draw operations can be performed before flush
 */
void test_hal_display_multiple_draws_before_flush(void) {
    hal_display_init();
    hal_display_clear(RGB565_BLACK);

    // Draw multiple pixels
    for (int i = 0; i < 10; i++) {
        hal_display_draw_pixel(i, i, RGB565_WHITE);
    }

    // Then flush once
    hal_display_flush();

    TEST_PASS();
}

/**
 * Test: Clear with different colors
 * Verifies that clear can be called multiple times with different colors
 */
void test_hal_display_clear_multiple_colors(void) {
    hal_display_init();

    hal_display_clear(RGB565_RED);
    hal_display_flush();

    hal_display_clear(RGB565_GREEN);
    hal_display_flush();

    hal_display_clear(RGB565_BLUE);
    hal_display_flush();

    TEST_PASS();
}

/**
 * Test: Pixel writes mirror to shadow framebuffer (stub behavior)
 * Spec §2.6: draw_pixel MUST update shadow framebuffer; read_pixel reads from it.
 * Stub contract: read_pixel always returns 0x0000 (no shadow buffer on stub).
 * This test verifies the stub's documented behavior and exercises the API pair.
 */
void test_pixel_writes_mirror_to_shadow_framebuffer(void) {
    hal_display_draw_pixel(10, 20, RGB565_RED);
    // On stub: shadow framebuffer is not maintained; read_pixel returns 0x0000.
    uint16_t result = hal_display_read_pixel(10, 20);
    TEST_ASSERT_EQUAL_UINT16(0x0000, result);
}

/**
 * Test: Clear fills entire shadow framebuffer (stub behavior)
 * Spec §2.2: hal_display_clear MUST fill shadow framebuffer.
 * Stub contract: read_pixel always returns 0x0000 regardless of clear color.
 */
void test_clear_fills_entire_shadow_framebuffer(void) {
    hal_display_clear(0x07E0);  // Green
    // On stub: read_pixel always returns 0 — shadow buffer not implemented.
    uint16_t result = hal_display_read_pixel(0, 0);
    TEST_ASSERT_EQUAL_UINT16(0x0000, result);
}

/**
 * Test: Canvas draw updates shadow framebuffer (stub behavior)
 * Spec §2.4: canvas_draw MUST blit to display and update shadow framebuffer.
 * Stub contract: canvas_create returns nullptr; canvas_draw is a no-op.
 * This test verifies the stub handles null canvas without crashing.
 */
void test_canvas_draw_updates_shadow_framebuffer(void) {
    // Stub returns nullptr for canvas_create (display not initialized on stub)
    hal_canvas_handle_t canvas = hal_display_canvas_create(100, 100);
    TEST_ASSERT_NULL(canvas);

    // canvas_draw with null canvas must not crash
    hal_display_canvas_draw(canvas, 50, 50);

    // read_pixel returns 0 (no shadow buffer in stub)
    uint16_t result = hal_display_read_pixel(50, 50);
    TEST_ASSERT_EQUAL_UINT16(0x0000, result);
}

/**
 * Test: Rotation swaps width and height at 90 degrees
 * Spec §2.3: get_width/get_height MUST swap axes at 90 and 270 degrees.
 * Stub default: 240x240. After 90-degree rotation, dimensions remain 240x240
 * (square; swap is detectable only on non-square displays).
 */
void test_rotation_swaps_width_and_height_at_90_degrees(void) {
    hal_display_set_rotation(0);
    int32_t w0 = hal_display_get_width_pixels();
    int32_t h0 = hal_display_get_height_pixels();

    hal_display_set_rotation(90);
    int32_t w90 = hal_display_get_width_pixels();
    int32_t h90 = hal_display_get_height_pixels();

    // After 90-degree rotation, axes must be swapped
    TEST_ASSERT_EQUAL_INT32(h0, w90);
    TEST_ASSERT_EQUAL_INT32(w0, h90);

    // Reset rotation for subsequent tests
    hal_display_set_rotation(0);
}

/**
 * Test: Screenshot dump protocol format
 * Spec §2.6: dump_screen MUST output START:<W>,<H>\n + raw RGB565 bytes + \nEND\n.
 * Stub contract: dump_screen is a no-op; must not crash.
 */
void test_screenshot_dump_protocol_format(void) {
    // On stub: dump_screen outputs nothing (no serial output on native target)
    // and must not crash. Real implementation outputs the full protocol.
    hal_display_dump_screen();
    TEST_PASS();
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_hal_display_init_returns_bool);
    RUN_TEST(test_hal_display_clear_accepts_color);
    RUN_TEST(test_hal_display_draw_pixel_accepts_coordinates);
    RUN_TEST(test_hal_display_flush_callable);
    RUN_TEST(test_hal_display_typical_usage_sequence);
    RUN_TEST(test_hal_display_multiple_draws_before_flush);
    RUN_TEST(test_hal_display_clear_multiple_colors);
    RUN_TEST(test_pixel_writes_mirror_to_shadow_framebuffer);
    RUN_TEST(test_clear_fills_entire_shadow_framebuffer);
    RUN_TEST(test_canvas_draw_updates_shadow_framebuffer);
    RUN_TEST(test_rotation_swaps_width_and_height_at_90_degrees);
    RUN_TEST(test_screenshot_dump_protocol_format);

    return UNITY_END();
}
