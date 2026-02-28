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
#include <Arduino_GFX_Library.h>
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
// Widget that captures/consumes input for routing tests
// ============================================================================

class InputCapturingWidget : public UIWidget {
public:
    bool consumed = false;

    void render(Arduino_GFX*, int32_t, int32_t, int32_t, int32_t) override {}

    bool handleInput(const touch_gesture_event_t&,
                     int32_t, int32_t, int32_t, int32_t) override {
        consumed = true;
        return true;
    }
};

// ============================================================================
// Recording GFX mock for render verification (background fill / underline)
// ============================================================================

class RecordingGFX : public Arduino_GFX {
public:
    int fillRectCalls = 0;
    int drawFastHLineCalls = 0;
    int16_t lastFillX = 0, lastFillY = 0, lastFillW = 0, lastFillH = 0;

    RecordingGFX() : Arduino_GFX(200, 200) {}

    bool begin(int32_t) override { return true; }
    void writePixelPreclipped(int16_t, int16_t, uint16_t) override {}

    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t) override {
        fillRectCalls++;
        lastFillX = x; lastFillY = y; lastFillW = w; lastFillH = h;
    }

    void drawFastHLine(int16_t, int16_t, int16_t, uint16_t) override {
        drawFastHLineCalls++;
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
// Widget minimum size enforcement (Scenario: Widget minimum size is enforced)
// ============================================================================

void test_widget_minimum_size_enforced() {
    GridWidgetLayout layout(1, 1);
    layout.setAnchorPoint(ANCHOR_TOP_LEFT);
    layout.setScreenRefPoint(ANCHOR_TOP_LEFT);
    layout.setOffset(0.0f, 0.0f);
    layout.setSize(0.3f, 0.3f); // 30% of 100x100 = 30x30

    TestWidget w;
    w.paddingX = 0; w.paddingY = 0;
    w.minWidth = 50; // Exceeds computed cell width of 30

    layout.addWidget(&w, 0, 0);
    layout.calculateLayout(100, 100);

    const WidgetCell* cell = layout.getCell(0);
    TEST_ASSERT_NOT_NULL(cell);
    // Cell width should be clamped to minWidth (50), not the computed 30
    TEST_ASSERT_EQUAL_INT32(50, cell->pixelW);
}

// ============================================================================
// Input hit-tests cells in reverse order (Scenario: Input hit-tests cells in reverse order)
// ============================================================================

void test_input_hit_tests_cells_in_reverse_order() {
    GridWidgetLayout layout(2, 2);
    layout.setAnchorPoint(ANCHOR_TOP_LEFT);
    layout.setScreenRefPoint(ANCHOR_TOP_LEFT);
    layout.setSize(1.0f, 1.0f);

    InputCapturingWidget w0, w1;
    w0.paddingX = 0; w0.paddingY = 0;
    w1.paddingX = 0; w1.paddingY = 0;

    // w0 spans the entire grid (2x2) — covers all pixels
    layout.addWidget(&w0, 0, 0, 2, 2);
    // w1 covers only the bottom-right cell (1,1) — added last
    layout.addWidget(&w1, 1, 1);

    layout.calculateLayout(100, 100);

    // Tap in the overlap area (bottom-right quarter: 75,75)
    touch_gesture_event_t tap = {};
    tap.type = TOUCH_TAP;
    tap.x_px = 75;
    tap.y_px = 75;

    bool consumed = layout.handleInput(tap);
    TEST_ASSERT_TRUE(consumed);
    // w1 (last-added) must have consumed it — w0 must NOT have received it
    TEST_ASSERT_TRUE(w1.consumed);
    TEST_ASSERT_FALSE(w0.consumed);
}

// ============================================================================
// ScrollableListWidget — missing scenario coverage
// ============================================================================

void test_add_item_returns_sequential_indices() {
    ScrollableListWidget list;
    int idx0 = list.addItem("Item 0");
    int idx1 = list.addItem("Item 1");
    int idx2 = list.addItem("Item 2");
    TEST_ASSERT_EQUAL_INT(0, idx0);
    TEST_ASSERT_EQUAL_INT(1, idx1);
    TEST_ASSERT_EQUAL_INT(2, idx2);
}

void test_swipe_up_scrolls_down_by_half_page() {
    ScrollableListWidget list;
    for (int i = 0; i < 20; i++) list.addItem("Item");
    // scrollOffset starts at 0; visible count in a 160px box with lineHeight 20 = 8 items
    // SWIPE_UP → scroll down by visibleCount/2 = 4
    touch_gesture_event_t swipe = {};
    swipe.type = TOUCH_SWIPE;
    swipe.direction = TOUCH_DIR_UP;
    swipe.x_px = 50; swipe.y_px = 80;
    list.handleInput(swipe, 0, 0, 100, 160);
    TEST_ASSERT_EQUAL_INT(4, list.getScrollOffset());
}

void test_scroll_clamps_to_bottom() {
    ScrollableListWidget list;
    for (int i = 0; i < 10; i++) list.addItem("Item");
    // 10 items, visible count = 5 (100px / 20px lineHeight), offset 4
    // SWIPE_UP → would add visibleCount/2=2 → 6, clamped to itemCount-visibleCount = 5
    touch_gesture_event_t swipe = {};
    swipe.type = TOUCH_SWIPE;
    swipe.direction = TOUCH_DIR_UP;
    swipe.x_px = 50; swipe.y_px = 50;

    // Get to offset 4 first
    list.handleInput(swipe, 0, 0, 100, 100); // → offset 2
    list.handleInput(swipe, 0, 0, 100, 100); // → offset 4
    list.handleInput(swipe, 0, 0, 100, 100); // → would be 6, clamped to 5 (10-5)
    TEST_ASSERT_EQUAL_INT(5, list.getScrollOffset());
}

// ============================================================================
// TextWidget — background fill and underline (Scenarios: Background fill / Underline)
// ============================================================================

void test_text_widget_background_fills_cell() {
    RecordingGFX gfx;
    TextWidget tw;
    tw.setText("Hello");
    tw.setBackgroundColor(0xF000);
    // Render into a 100x50 cell at (0,0)
    tw.render(&gfx, 0, 0, 100, 50);
    // fillRect must have been called once with the full bounding box
    TEST_ASSERT_EQUAL_INT(1, gfx.fillRectCalls);
    TEST_ASSERT_EQUAL_INT16(0, gfx.lastFillX);
    TEST_ASSERT_EQUAL_INT16(0, gfx.lastFillY);
    TEST_ASSERT_EQUAL_INT16(100, gfx.lastFillW);
    TEST_ASSERT_EQUAL_INT16(50, gfx.lastFillH);
}

void test_text_widget_underline_draws_below_text() {
    RecordingGFX gfx;
    TextWidget tw;
    tw.setText("Hello");
    tw.setUnderlined(true);
    tw.render(&gfx, 0, 0, 100, 50);
    // drawFastHLine must have been called for the underline
    TEST_ASSERT_GREATER_THAN(0, gfx.drawFastHLineCalls);
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

    // Widget minimum size enforcement
    RUN_TEST(test_widget_minimum_size_enforced);

    // Input reverse-order hit testing
    RUN_TEST(test_input_hit_tests_cells_in_reverse_order);

    // ScrollableList — missing scenarios
    RUN_TEST(test_add_item_returns_sequential_indices);
    RUN_TEST(test_swipe_up_scrolls_down_by_half_page);
    RUN_TEST(test_scroll_clamps_to_bottom);

    // TextWidget background fill and underline
    RUN_TEST(test_text_widget_background_fills_cell);
    RUN_TEST(test_text_widget_underline_draws_below_text);

    return UNITY_END();
}
