/**
 * @file test_relative_display.cpp
 * @brief Unity tests for RelativeDisplay class
 *
 * These tests verify the object-oriented RelativeDisplay class that wraps
 * an Arduino_GFX canvas/display and provides relative coordinate drawing.
 */

#include <unity.h>
#include "../../src/relative_display.h"
#include <cmath>
#include <vector>

// RGB565 color definitions
#define RGB565_BLACK   0x0000
#define RGB565_WHITE   0xFFFF
#define RGB565_RED     0xF800
#define RGB565_GREEN   0x07E0
#define RGB565_BLUE    0x001F

// Mock Arduino_GFX class for testing
class MockArduinoGFX : public Arduino_GFX {
public:
    struct DrawCall {
        enum Type { PIXEL, HLINE, VLINE, FILLRECT };
        Type type;
        int16_t x, y, w, h;
        uint16_t color;
    };

    std::vector<DrawCall> calls;

    MockArduinoGFX(int16_t w, int16_t h) : Arduino_GFX(w, h) {}

    bool begin(int32_t speed = 0) override { return true; }
    void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {}

    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        DrawCall call;
        call.type = DrawCall::PIXEL;
        call.x = x;
        call.y = y;
        call.color = color;
        calls.push_back(call);
    }

    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override {
        DrawCall call;
        call.type = DrawCall::HLINE;
        call.x = x;
        call.y = y;
        call.w = w;
        call.color = color;
        calls.push_back(call);
    }

    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override {
        DrawCall call;
        call.type = DrawCall::VLINE;
        call.x = x;
        call.y = y;
        call.h = h;
        call.color = color;
        calls.push_back(call);
    }

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override {
        DrawCall call;
        call.type = DrawCall::FILLRECT;
        call.x = x;
        call.y = y;
        call.w = w;
        call.h = h;
        call.color = color;
        calls.push_back(call);
    }

    void clearCalls() {
        calls.clear();
    }
};

// Global test fixtures
MockArduinoGFX* mockGfx = nullptr;
RelativeDisplay* relDisplay = nullptr;

void setUp(void) {
    // Create a 200x200 mock display for testing
    mockGfx = new MockArduinoGFX(200, 200);
    relDisplay = new RelativeDisplay(mockGfx, 200, 200);
}

void tearDown(void) {
    delete relDisplay;
    delete mockGfx;
    relDisplay = nullptr;
    mockGfx = nullptr;
}

/**
 * Test: Coordinate Conversion - X axis
 * Verifies relativeToAbsoluteX conversion for a 200x200 surface
 */
void test_relative_to_absolute_x(void) {
    TEST_ASSERT_EQUAL_INT32(0, relDisplay->relativeToAbsoluteX(0.0f));
    TEST_ASSERT_EQUAL_INT32(50, relDisplay->relativeToAbsoluteX(25.0f));
    TEST_ASSERT_EQUAL_INT32(100, relDisplay->relativeToAbsoluteX(50.0f));
    TEST_ASSERT_EQUAL_INT32(150, relDisplay->relativeToAbsoluteX(75.0f));
    TEST_ASSERT_EQUAL_INT32(200, relDisplay->relativeToAbsoluteX(100.0f));
}

/**
 * Test: Coordinate Conversion - Y axis
 * Verifies relativeToAbsoluteY conversion for a 200x200 surface
 */
void test_relative_to_absolute_y(void) {
    TEST_ASSERT_EQUAL_INT32(0, relDisplay->relativeToAbsoluteY(0.0f));
    TEST_ASSERT_EQUAL_INT32(50, relDisplay->relativeToAbsoluteY(25.0f));
    TEST_ASSERT_EQUAL_INT32(100, relDisplay->relativeToAbsoluteY(50.0f));
    TEST_ASSERT_EQUAL_INT32(150, relDisplay->relativeToAbsoluteY(75.0f));
    TEST_ASSERT_EQUAL_INT32(200, relDisplay->relativeToAbsoluteY(100.0f));
}

/**
 * Test: Coordinate Conversion - Width
 * Verifies relativeToAbsoluteWidth conversion for a 200x200 surface
 */
void test_relative_to_absolute_width(void) {
    TEST_ASSERT_EQUAL_INT32(0, relDisplay->relativeToAbsoluteWidth(0.0f));
    TEST_ASSERT_EQUAL_INT32(50, relDisplay->relativeToAbsoluteWidth(25.0f));
    TEST_ASSERT_EQUAL_INT32(100, relDisplay->relativeToAbsoluteWidth(50.0f));
    TEST_ASSERT_EQUAL_INT32(150, relDisplay->relativeToAbsoluteWidth(75.0f));
    TEST_ASSERT_EQUAL_INT32(200, relDisplay->relativeToAbsoluteWidth(100.0f));
}

/**
 * Test: Coordinate Conversion - Height
 * Verifies relativeToAbsoluteHeight conversion for a 200x200 surface
 */
void test_relative_to_absolute_height(void) {
    TEST_ASSERT_EQUAL_INT32(0, relDisplay->relativeToAbsoluteHeight(0.0f));
    TEST_ASSERT_EQUAL_INT32(50, relDisplay->relativeToAbsoluteHeight(25.0f));
    TEST_ASSERT_EQUAL_INT32(100, relDisplay->relativeToAbsoluteHeight(50.0f));
    TEST_ASSERT_EQUAL_INT32(150, relDisplay->relativeToAbsoluteHeight(75.0f));
    TEST_ASSERT_EQUAL_INT32(200, relDisplay->relativeToAbsoluteHeight(100.0f));
}

/**
 * Test: Draw Single Pixel
 * Verifies that drawPixel calls the underlying GFX object correctly
 */
void test_draw_pixel(void) {
    relDisplay->drawPixel(50.0f, 50.0f, RGB565_RED);

    TEST_ASSERT_EQUAL_UINT32(1, mockGfx->calls.size());
    TEST_ASSERT_EQUAL(MockArduinoGFX::DrawCall::PIXEL, mockGfx->calls[0].type);
    TEST_ASSERT_EQUAL_INT16(100, mockGfx->calls[0].x);
    TEST_ASSERT_EQUAL_INT16(100, mockGfx->calls[0].y);
    TEST_ASSERT_EQUAL_UINT16(RGB565_RED, mockGfx->calls[0].color);
}

/**
 * Test: Draw Horizontal Line
 * Verifies that drawHorizontalLine calls the underlying GFX object correctly
 */
void test_draw_horizontal_line(void) {
    relDisplay->drawHorizontalLine(50.0f, 25.0f, 75.0f, RGB565_GREEN);

    TEST_ASSERT_EQUAL_UINT32(1, mockGfx->calls.size());
    TEST_ASSERT_EQUAL(MockArduinoGFX::DrawCall::HLINE, mockGfx->calls[0].type);
    TEST_ASSERT_EQUAL_INT16(50, mockGfx->calls[0].x);  // 25% of 200
    TEST_ASSERT_EQUAL_INT16(100, mockGfx->calls[0].y); // 50% of 200
    TEST_ASSERT_EQUAL_INT16(101, mockGfx->calls[0].w); // 75% - 25% + 1 = 100 + 1
    TEST_ASSERT_EQUAL_UINT16(RGB565_GREEN, mockGfx->calls[0].color);
}

/**
 * Test: Draw Vertical Line
 * Verifies that drawVerticalLine calls the underlying GFX object correctly
 */
void test_draw_vertical_line(void) {
    relDisplay->drawVerticalLine(50.0f, 25.0f, 75.0f, RGB565_BLUE);

    TEST_ASSERT_EQUAL_UINT32(1, mockGfx->calls.size());
    TEST_ASSERT_EQUAL(MockArduinoGFX::DrawCall::VLINE, mockGfx->calls[0].type);
    TEST_ASSERT_EQUAL_INT16(100, mockGfx->calls[0].x); // 50% of 200
    TEST_ASSERT_EQUAL_INT16(50, mockGfx->calls[0].y);  // 25% of 200
    TEST_ASSERT_EQUAL_INT16(101, mockGfx->calls[0].h); // 75% - 25% + 1 = 100 + 1
    TEST_ASSERT_EQUAL_UINT16(RGB565_BLUE, mockGfx->calls[0].color);
}

/**
 * Test: Fill Rectangle (Scenario from feature file)
 * Given a 200x200 pixel surface
 * When fillRect(10.0f, 10.0f, 80.0f, 80.0f, 0xFFFF) is called
 * Then the underlying GFX fillRect should be called with (20, 20, 160, 160, 0xFFFF)
 */
void test_fill_rect_scenario(void) {
    relDisplay->fillRect(10.0f, 10.0f, 80.0f, 80.0f, 0xFFFF);

    TEST_ASSERT_EQUAL_UINT32(1, mockGfx->calls.size());
    TEST_ASSERT_EQUAL(MockArduinoGFX::DrawCall::FILLRECT, mockGfx->calls[0].type);
    TEST_ASSERT_EQUAL_INT16(20, mockGfx->calls[0].x);  // 10% of 200
    TEST_ASSERT_EQUAL_INT16(20, mockGfx->calls[0].y);  // 10% of 200
    TEST_ASSERT_EQUAL_INT16(160, mockGfx->calls[0].w); // 80% of 200
    TEST_ASSERT_EQUAL_INT16(160, mockGfx->calls[0].h); // 80% of 200
    TEST_ASSERT_EQUAL_UINT16(0xFFFF, mockGfx->calls[0].color);
}

/**
 * Test: Get GFX pointer
 * Verifies that getGfx returns the correct underlying GFX object
 */
void test_get_gfx(void) {
    Arduino_GFX* gfx = relDisplay->getGfx();
    TEST_ASSERT_EQUAL_PTR(mockGfx, gfx);
}

/**
 * Test: Different surface dimensions
 * Verifies conversion works correctly with non-square dimensions
 */
void test_non_square_surface(void) {
    delete relDisplay;
    delete mockGfx;

    // Create a 240x536 surface (like T-Display-S3-Plus)
    mockGfx = new MockArduinoGFX(240, 536);
    relDisplay = new RelativeDisplay(mockGfx, 240, 536);

    // Test conversions
    TEST_ASSERT_EQUAL_INT32(120, relDisplay->relativeToAbsoluteX(50.0f));  // 50% of 240
    TEST_ASSERT_EQUAL_INT32(268, relDisplay->relativeToAbsoluteY(50.0f));  // 50% of 536
    TEST_ASSERT_EQUAL_INT32(60, relDisplay->relativeToAbsoluteWidth(25.0f));   // 25% of 240
    TEST_ASSERT_EQUAL_INT32(134, relDisplay->relativeToAbsoluteHeight(25.0f)); // 25% of 536

    // Test drawing
    relDisplay->fillRect(10.0f, 10.0f, 80.0f, 80.0f, RGB565_WHITE);

    TEST_ASSERT_EQUAL_UINT32(1, mockGfx->calls.size());
    TEST_ASSERT_EQUAL_INT16(24, mockGfx->calls[0].x);   // 10% of 240
    TEST_ASSERT_EQUAL_INT16(54, mockGfx->calls[0].y);   // 10% of 536 (rounded)
    TEST_ASSERT_EQUAL_INT16(192, mockGfx->calls[0].w);  // 80% of 240
    TEST_ASSERT_EQUAL_INT16(429, mockGfx->calls[0].h);  // 80% of 536 (rounded)
}

/**
 * Test: Draw Solid Background (features/display_background.md)
 * Scenario: Drawing a Solid Background
 * Given the RelativeDisplay is initialized
 * When drawSolidBackground(0xF800) (Red) is called
 * Then the entire drawing area should be filled with red
 */
void test_draw_solid_background(void) {
    relDisplay->drawSolidBackground(RGB565_RED);

    TEST_ASSERT_EQUAL_UINT32(1, mockGfx->calls.size());
    TEST_ASSERT_EQUAL(MockArduinoGFX::DrawCall::FILLRECT, mockGfx->calls[0].type);
    TEST_ASSERT_EQUAL_INT16(0, mockGfx->calls[0].x);    // 0% of width
    TEST_ASSERT_EQUAL_INT16(0, mockGfx->calls[0].y);    // 0% of height
    TEST_ASSERT_EQUAL_INT16(200, mockGfx->calls[0].w);  // 100% of width (200px)
    TEST_ASSERT_EQUAL_INT16(200, mockGfx->calls[0].h);  // 100% of height (200px)
    TEST_ASSERT_EQUAL_UINT16(RGB565_RED, mockGfx->calls[0].color);
}

/**
 * Test: C-style solid background wrapper
 * Verifies the backward-compatible C API for solid backgrounds
 */
void test_c_style_solid_background(void) {
    // Initialize the procedural API with our mock
    display_relative_init();

    // Call the C-style function
    display_relative_draw_solid_background(RGB565_BLUE);

    // The procedural API should have called fillRect on the entire surface
    // Note: This test verifies the function can be called; full verification
    // would require HAL mocking which is beyond the scope of this unit test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_relative_to_absolute_x);
    RUN_TEST(test_relative_to_absolute_y);
    RUN_TEST(test_relative_to_absolute_width);
    RUN_TEST(test_relative_to_absolute_height);
    RUN_TEST(test_draw_pixel);
    RUN_TEST(test_draw_horizontal_line);
    RUN_TEST(test_draw_vertical_line);
    RUN_TEST(test_fill_rect_scenario);
    RUN_TEST(test_get_gfx);
    RUN_TEST(test_non_square_surface);
    RUN_TEST(test_draw_solid_background);
    RUN_TEST(test_c_style_solid_background);

    return UNITY_END();
}
