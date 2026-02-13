/**
 * @file test_widget_framework.cpp
 * @brief Unit tests for the Widget Framework
 *
 * Tests GridWidgetLayout anchor math, cell subdivision,
 * WidgetLayoutEngine coordination, and ScrollableListWidget logic.
 *
 * Specification: features/ui_widget_framework.md
 */

#include <unity.h>
#include "ui/widgets/ui_widget.h"
#include "ui/widgets/text_widget.h"
#include "ui/widgets/scrollable_list_widget.h"

// ============================================================================
// Concrete test widget for layout verification
// ============================================================================

class TestWidget : public UIWidget {
public:
    int renderCallCount = 0;
    int32_t lastX = 0, lastY = 0, lastW = 0, lastH = 0;

    void render(Arduino_GFX* gfx, int32_t x, int32_t y, int32_t w, int32_t h) override {
        (void)gfx;
        renderCallCount++;
        lastX = x; lastY = y; lastW = w; lastH = h;
    }
};

// ============================================================================
// GridWidgetLayout - Anchor & Position Tests
// ============================================================================

void test_anchor_top_center_positioning() {
    // Spec scenario: 1x5 grid, anchored TOP_CENTER, 10% down from TOP_CENTER, 50% size
    GridWidgetLayout layout(5, 1);
    layout.setAnchorPoint(ANCHOR_TOP_CENTER);
    layout.setScreenRefPoint(ANCHOR_TOP_CENTER);
    layout.setOffset(0.0f, 0.10f);
    layout.setSize(0.50f, 0.50f);

    layout.calculateLayout(480, 480);

    // Layout should be centered horizontally
    // screenRef TOP_CENTER = (240, 0), offset (0, 0.10) -> target (240, 48)
    // Layout size = (240, 240)
    // Anchor TOP_CENTER -> layout x = 240 - 120 = 120, y = 48
    TEST_ASSERT_EQUAL_INT32(120, layout.getPixelX());
    TEST_ASSERT_EQUAL_INT32(48, layout.getPixelY());
    TEST_ASSERT_EQUAL_INT32(240, layout.getPixelW());
    TEST_ASSERT_EQUAL_INT32(240, layout.getPixelH());
}

void test_anchor_center_positioning() {
    GridWidgetLayout layout(2, 2);
    layout.setAnchorPoint(ANCHOR_CENTER);
    layout.setScreenRefPoint(ANCHOR_CENTER);
    layout.setOffset(0.0f, 0.0f);
    layout.setSize(0.50f, 0.50f);

    layout.calculateLayout(400, 300);

    // CENTER of 400x300 = (200, 150)
    // Layout = 200x150, anchor CENTER -> x = 200-100 = 100, y = 150-75 = 75
    TEST_ASSERT_EQUAL_INT32(100, layout.getPixelX());
    TEST_ASSERT_EQUAL_INT32(75, layout.getPixelY());
    TEST_ASSERT_EQUAL_INT32(200, layout.getPixelW());
    TEST_ASSERT_EQUAL_INT32(150, layout.getPixelH());
}

void test_anchor_bottom_right_positioning() {
    GridWidgetLayout layout(1, 1);
    layout.setAnchorPoint(ANCHOR_BOTTOM_RIGHT);
    layout.setScreenRefPoint(ANCHOR_BOTTOM_RIGHT);
    layout.setOffset(0.0f, 0.0f);
    layout.setSize(0.25f, 0.25f);

    layout.calculateLayout(800, 600);

    // BOTTOM_RIGHT of 800x600 = (800, 600)
    // Layout = 200x150, anchor BOTTOM_RIGHT -> x = 800-200 = 600, y = 600-150 = 450
    TEST_ASSERT_EQUAL_INT32(600, layout.getPixelX());
    TEST_ASSERT_EQUAL_INT32(450, layout.getPixelY());
}

// ============================================================================
// GridWidgetLayout - Cell Subdivision Tests
// ============================================================================

void test_grid_cell_subdivision_1x5() {
    GridWidgetLayout layout(5, 1);
    layout.setAnchorPoint(ANCHOR_TOP_LEFT);
    layout.setScreenRefPoint(ANCHOR_TOP_LEFT);
    layout.setOffset(0.0f, 0.0f);
    layout.setSize(1.0f, 1.0f);

    TestWidget w0, w1;
    w0.paddingX = 0; w0.paddingY = 0;
    w1.paddingX = 0; w1.paddingY = 0;

    layout.addWidget(&w0, 0, 0);
    layout.addWidget(&w1, 1, 0, 4, 1);  // Spans rows 1-4

    layout.calculateLayout(200, 500);

    // Cell height = 500 / 5 = 100
    // w0: row 0, col 0 -> (0, 0, 200, 100)
    const WidgetCell* cell0 = layout.getCell(0);
    TEST_ASSERT_NOT_NULL(cell0);
    TEST_ASSERT_EQUAL_INT32(0, cell0->pixelX);
    TEST_ASSERT_EQUAL_INT32(0, cell0->pixelY);
    TEST_ASSERT_EQUAL_INT32(200, cell0->pixelW);
    TEST_ASSERT_EQUAL_INT32(100, cell0->pixelH);

    // w1: row 1, col 0, span 4 rows -> (0, 100, 200, 400)
    const WidgetCell* cell1 = layout.getCell(1);
    TEST_ASSERT_NOT_NULL(cell1);
    TEST_ASSERT_EQUAL_INT32(0, cell1->pixelX);
    TEST_ASSERT_EQUAL_INT32(100, cell1->pixelY);
    TEST_ASSERT_EQUAL_INT32(200, cell1->pixelW);
    TEST_ASSERT_EQUAL_INT32(400, cell1->pixelH);
}

void test_grid_cell_with_padding() {
    GridWidgetLayout layout(2, 2);
    layout.setAnchorPoint(ANCHOR_TOP_LEFT);
    layout.setScreenRefPoint(ANCHOR_TOP_LEFT);
    layout.setOffset(0.0f, 0.0f);
    layout.setSize(1.0f, 1.0f);

    TestWidget w0;
    w0.paddingX = 5;
    w0.paddingY = 10;

    layout.addWidget(&w0, 0, 0);
    layout.calculateLayout(200, 200);

    // Cell size = 100x100, with padding (5,10):
    // x = 0 + 5 = 5, y = 0 + 10 = 10
    // w = 100 - 10 = 90, h = 100 - 20 = 80
    const WidgetCell* cell = layout.getCell(0);
    TEST_ASSERT_EQUAL_INT32(5, cell->pixelX);
    TEST_ASSERT_EQUAL_INT32(10, cell->pixelY);
    TEST_ASSERT_EQUAL_INT32(90, cell->pixelW);
    TEST_ASSERT_EQUAL_INT32(80, cell->pixelH);
}

// ============================================================================
// WidgetLayoutEngine Tests
// ============================================================================

void test_widget_engine_manages_layouts() {
    WidgetLayoutEngine engine;
    TEST_ASSERT_EQUAL_INT(0, engine.getLayoutCount());

    GridWidgetLayout layout1(1, 1);
    GridWidgetLayout layout2(2, 2);

    engine.addLayout(&layout1);
    engine.addLayout(&layout2);

    TEST_ASSERT_EQUAL_INT(2, engine.getLayoutCount());
}

void test_widget_engine_render_calls_widgets() {
    WidgetLayoutEngine engine;
    GridWidgetLayout layout(1, 1);
    layout.setAnchorPoint(ANCHOR_TOP_LEFT);
    layout.setScreenRefPoint(ANCHOR_TOP_LEFT);
    layout.setSize(1.0f, 1.0f);

    TestWidget w;
    w.paddingX = 0; w.paddingY = 0;
    layout.addWidget(&w, 0, 0);

    engine.addLayout(&layout);
    engine.calculateLayouts(100, 100);
    engine.render(nullptr);  // GFX is nullptr — TestWidget ignores it

    TEST_ASSERT_EQUAL_INT(1, w.renderCallCount);
    TEST_ASSERT_EQUAL_INT32(0, w.lastX);
    TEST_ASSERT_EQUAL_INT32(0, w.lastY);
    TEST_ASSERT_EQUAL_INT32(100, w.lastW);
    TEST_ASSERT_EQUAL_INT32(100, w.lastH);
}

// ============================================================================
// ScrollableListWidget Tests
// ============================================================================

void test_scrollable_list_add_items() {
    ScrollableListWidget list;
    TEST_ASSERT_EQUAL_INT(0, list.getItemCount());

    list.addItem("Item 1");
    list.addItem("Item 2");
    list.addItem("Item 3");

    TEST_ASSERT_EQUAL_INT(3, list.getItemCount());
}

void test_scrollable_list_clear() {
    ScrollableListWidget list;
    list.addItem("Item 1");
    list.addItem("Item 2");
    list.clearItems();
    TEST_ASSERT_EQUAL_INT(0, list.getItemCount());
}

void test_scrollable_list_scroll_bounds() {
    ScrollableListWidget list;
    for (int i = 0; i < 20; i++) {
        list.addItem("Item");
    }

    // Simulate downward scroll (should go negative, clamped to 0)
    touch_gesture_event_t event = {};
    event.type = TOUCH_SWIPE;
    event.direction = TOUCH_DIR_DOWN;
    event.x_px = 50;
    event.y_px = 50;

    list.handleInput(event, 0, 0, 100, 200);
    TEST_ASSERT_EQUAL_INT(0, list.getScrollOffset());  // Already at top
}

void test_scrollable_list_selection() {
    ScrollableListWidget list;
    list.addItem("Item 0");
    list.addItem("Item 1");
    list.addItem("Item 2");

    // No selection initially
    TEST_ASSERT_EQUAL_INT(-1, list.getSelectedIndex());

    // Tap at y=30 with lineHeight=20 → item 1
    // Note: in native test, lineHeight defaults to 20 (font returns 0)
    touch_gesture_event_t tap = {};
    tap.type = TOUCH_TAP;
    tap.x_px = 50;
    tap.y_px = 30;  // item index = 30 / 20 = 1

    list.handleInput(tap, 0, 0, 100, 100);
    TEST_ASSERT_EQUAL_INT(1, list.getSelectedIndex());
}

// ============================================================================
// Hit Testing - Layout Input Routing
// ============================================================================

void test_layout_input_hit_test() {
    GridWidgetLayout layout(1, 2);
    layout.setAnchorPoint(ANCHOR_TOP_LEFT);
    layout.setScreenRefPoint(ANCHOR_TOP_LEFT);
    layout.setSize(1.0f, 1.0f);

    TestWidget w0, w1;
    w0.paddingX = 0; w0.paddingY = 0;
    w1.paddingX = 0; w1.paddingY = 0;

    // Two columns: w0 in col 0, w1 in col 1
    layout.addWidget(&w0, 0, 0);
    layout.addWidget(&w1, 0, 1);

    layout.calculateLayout(200, 100);

    // w0 should be at (0, 0, 100, 100)
    // w1 should be at (100, 0, 100, 100)
    const WidgetCell* c0 = layout.getCell(0);
    const WidgetCell* c1 = layout.getCell(1);
    TEST_ASSERT_EQUAL_INT32(0, c0->pixelX);
    TEST_ASSERT_EQUAL_INT32(100, c1->pixelX);
}

// ============================================================================
// TextWidget Tests
// ============================================================================

void test_text_widget_creation() {
    TextWidget tw;
    tw.setText("Hello");
    tw.setColor(0xFFFF);
    tw.justificationX = JUSTIFY_CENTER_X;
    tw.justificationY = JUSTIFY_CENTER_Y;

    // Just verify it doesn't crash with nullptr GFX
    // (mock GFX returns 0 for all text bounds)
    tw.render(nullptr, 0, 0, 100, 50);
}

// ============================================================================
// Test Runner
// ============================================================================

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // Layout anchor tests
    RUN_TEST(test_anchor_top_center_positioning);
    RUN_TEST(test_anchor_center_positioning);
    RUN_TEST(test_anchor_bottom_right_positioning);

    // Grid cell tests
    RUN_TEST(test_grid_cell_subdivision_1x5);
    RUN_TEST(test_grid_cell_with_padding);

    // Engine tests
    RUN_TEST(test_widget_engine_manages_layouts);
    RUN_TEST(test_widget_engine_render_calls_widgets);

    // Scrollable list tests
    RUN_TEST(test_scrollable_list_add_items);
    RUN_TEST(test_scrollable_list_clear);
    RUN_TEST(test_scrollable_list_scroll_bounds);
    RUN_TEST(test_scrollable_list_selection);

    // Hit testing
    RUN_TEST(test_layout_input_hit_test);

    // TextWidget
    RUN_TEST(test_text_widget_creation);

    return UNITY_END();
}
