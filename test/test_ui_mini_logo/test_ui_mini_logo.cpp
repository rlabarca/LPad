#include <unity.h>
#include "ui_mini_logo.h"
#include "relative_display.h"
#include "../hal/display.h"
#include <Arduino_GFX_Library.h>

// Test display dimensions
static constexpr int32_t TEST_WIDTH = 320;
static constexpr int32_t TEST_HEIGHT = 170;

// Mock GFX class for testing
class MockGFX : public Arduino_GFX {
public:
    MockGFX() : Arduino_GFX(TEST_WIDTH, TEST_HEIGHT) {}

    bool begin(int32_t speed = 0) override { return true; }
    void writePixelPreclipped(int16_t x, int16_t y, uint16_t color) override {}
};

static MockGFX* gfx = nullptr;
static RelativeDisplay* display = nullptr;

void setUp() {
    // Initialize HAL
    hal_display_init();
    hal_display_set_rotation(1);

    // Create mock display
    gfx = new MockGFX();
    display = new RelativeDisplay(gfx, TEST_WIDTH, TEST_HEIGHT);
}

void tearDown() {
    delete display;
    delete gfx;
    display = nullptr;
    gfx = nullptr;
}

// Test: MiniLogo can be instantiated for each corner
void test_minilogo_instantiation() {
    MiniLogo logo_tl(display, MiniLogo::Corner::TOP_LEFT);
    TEST_ASSERT_EQUAL(MiniLogo::Corner::TOP_LEFT, logo_tl.getCorner());

    MiniLogo logo_tr(display, MiniLogo::Corner::TOP_RIGHT);
    TEST_ASSERT_EQUAL(MiniLogo::Corner::TOP_RIGHT, logo_tr.getCorner());

    MiniLogo logo_bl(display, MiniLogo::Corner::BOTTOM_LEFT);
    TEST_ASSERT_EQUAL(MiniLogo::Corner::BOTTOM_LEFT, logo_bl.getCorner());

    MiniLogo logo_br(display, MiniLogo::Corner::BOTTOM_RIGHT);
    TEST_ASSERT_EQUAL(MiniLogo::Corner::BOTTOM_RIGHT, logo_br.getCorner());
}

// Test: Corner position can be changed
void test_minilogo_set_corner() {
    MiniLogo logo(display, MiniLogo::Corner::TOP_LEFT);
    TEST_ASSERT_EQUAL(MiniLogo::Corner::TOP_LEFT, logo.getCorner());

    logo.setCorner(MiniLogo::Corner::BOTTOM_RIGHT);
    TEST_ASSERT_EQUAL(MiniLogo::Corner::BOTTOM_RIGHT, logo.getCorner());

    logo.setCorner(MiniLogo::Corner::TOP_RIGHT);
    TEST_ASSERT_EQUAL(MiniLogo::Corner::TOP_RIGHT, logo.getCorner());
}

// Test: render() can be called without crashing (basic smoke test)
void test_minilogo_render_smoke() {
    MiniLogo logo(display, MiniLogo::Corner::TOP_RIGHT);
    // Should not crash
    logo.render();
    TEST_PASS();
}

// Test: render() with null display doesn't crash
void test_minilogo_render_null_display() {
    MiniLogo logo(nullptr, MiniLogo::Corner::TOP_RIGHT);
    // Should not crash
    logo.render();
    TEST_PASS();
}

// Test: All corners can be rendered without crashing
void test_minilogo_render_all_corners() {
    MiniLogo logo(display, MiniLogo::Corner::TOP_LEFT);
    logo.render();

    logo.setCorner(MiniLogo::Corner::TOP_RIGHT);
    logo.render();

    logo.setCorner(MiniLogo::Corner::BOTTOM_LEFT);
    logo.render();

    logo.setCorner(MiniLogo::Corner::BOTTOM_RIGHT);
    logo.render();

    TEST_PASS();
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_minilogo_instantiation);
    RUN_TEST(test_minilogo_set_corner);
    RUN_TEST(test_minilogo_render_smoke);
    RUN_TEST(test_minilogo_render_null_display);
    RUN_TEST(test_minilogo_render_all_corners);

    return UNITY_END();
}
