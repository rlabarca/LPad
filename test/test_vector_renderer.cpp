#include <unity.h>
#include "vector_renderer.h"
#include "generated/vector_assets.h"
#include "relative_display.h"
#include "hal/display.h"

// Mock display for testing
static Arduino_GFX* g_test_gfx = nullptr;
static RelativeDisplay* g_test_display = nullptr;

void setUp(void) {
    // Initialize HAL display (uses stub on native platform)
    hal_display_init();
    g_test_gfx = hal_display_get_gfx();
    g_test_display = new RelativeDisplay(g_test_gfx, 320, 170);
    g_test_display->init();
}

void tearDown(void) {
    delete g_test_display;
    g_test_display = nullptr;
    g_test_gfx = nullptr;
}

// Test: Generated assets are available
void test_vector_assets_available(void) {
    TEST_ASSERT_NOT_NULL(VectorAssets::Lpadlogo.paths);
    TEST_ASSERT_GREATER_THAN(0, VectorAssets::Lpadlogo.num_paths);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 245.0f, VectorAssets::Lpadlogo.original_width);
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 370.0f, VectorAssets::Lpadlogo.original_height);
}

// Test: Generated assets have valid triangle data
void test_vector_assets_triangles(void) {
    const VectorShape& logo = VectorAssets::Lpadlogo;

    // LPadLogo has 10 paths (triangles)
    TEST_ASSERT_EQUAL(10, logo.num_paths);

    // Check first path
    TEST_ASSERT_EQUAL(1, logo.paths[0].num_tris);
    TEST_ASSERT_NOT_NULL(logo.paths[0].tris);

    // Verify vertices are normalized [0,1]
    const VectorTriangle& tri = logo.paths[0].tris[0];
    TEST_ASSERT_GREATER_OR_EQUAL(0.0f, tri.v1.x);
    TEST_ASSERT_LESS_OR_EQUAL(1.0f, tri.v1.x);
    TEST_ASSERT_GREATER_OR_EQUAL(0.0f, tri.v1.y);
    TEST_ASSERT_LESS_OR_EQUAL(1.0f, tri.v1.y);
}

// Test: Basic rendering call (smoke test)
void test_vector_renderer_draw(void) {
    // This is a smoke test - just verify the method can be called without crashing
    VectorRenderer::draw(
        *g_test_display,
        VectorAssets::Lpadlogo,
        50.0f,  // Center X
        50.0f,  // Center Y
        20.0f,  // 20% width
        0.5f,   // Center anchor X
        0.5f    // Center anchor Y
    );

    // If we get here without crashing, test passes
    TEST_PASS();
}

// Test: Rendering at different positions
void test_vector_renderer_positioning(void) {
    // Top-left corner
    VectorRenderer::draw(*g_test_display, VectorAssets::Lpadlogo,
                        10.0f, 10.0f, 15.0f, 0.0f, 0.0f);

    // Bottom-right corner
    VectorRenderer::draw(*g_test_display, VectorAssets::Lpadlogo,
                        90.0f, 90.0f, 15.0f, 1.0f, 1.0f);

    // Center with large size
    VectorRenderer::draw(*g_test_display, VectorAssets::Lpadlogo,
                        50.0f, 50.0f, 40.0f, 0.5f, 0.5f);

    TEST_PASS();
}

// Test: Color conversion (RGB565)
void test_vector_assets_colors(void) {
    const VectorShape& logo = VectorAssets::Lpadlogo;

    // First path should have color #6A6556 -> RGB565
    // R=0x6A (106) -> 106>>3 = 13 (0xD)
    // G=0x65 (101) -> 101>>2 = 25 (0x19)
    // B=0x56 (86)  -> 86>>3  = 10 (0xA)
    // RGB565 = 0xD<<11 | 0x19<<5 | 0xA = 0x6B2A
    TEST_ASSERT_EQUAL_HEX16(0x6B2A, logo.paths[0].color);
}

void process(void) {
    UNITY_BEGIN();
    RUN_TEST(test_vector_assets_available);
    RUN_TEST(test_vector_assets_triangles);
    RUN_TEST(test_vector_assets_colors);
    RUN_TEST(test_vector_renderer_draw);
    RUN_TEST(test_vector_renderer_positioning);
    UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
void setup() {
    delay(2000);
    process();
}
void loop() {}
#else
int main(int argc, char **argv) {
    process();
    return 0;
}
#endif
