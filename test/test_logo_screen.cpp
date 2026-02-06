#include <unity.h>
#include "ui_logo_screen.h"
#include "relative_display.h"
#include "hal/display.h"

static Arduino_GFX* g_test_gfx = nullptr;
static RelativeDisplay* g_test_display = nullptr;

void setUp(void) {
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

// Test: Initial state is WAIT
void test_logo_screen_initial_state(void) {
    LogoScreen logo(2.0f, 1.5f);
    logo.begin(g_test_display, 0x0000);
    TEST_ASSERT_EQUAL(LogoScreen::State::WAIT, logo.getState());
    TEST_ASSERT_FALSE(logo.isDone());
}

// Test: Transition from WAIT to ANIMATE
void test_logo_screen_wait_to_animate(void) {
    LogoScreen logo(2.0f, 1.5f);
    logo.begin(g_test_display, 0x0000);

    // Update with small deltaTime - should stay in WAIT
    logo.update(0.5f);
    TEST_ASSERT_EQUAL(LogoScreen::State::WAIT, logo.getState());

    // Update to exceed wait duration - should transition to ANIMATE
    logo.update(1.6f);  // Total: 2.1s
    TEST_ASSERT_EQUAL(LogoScreen::State::ANIMATE, logo.getState());
}

// Test: Transition from ANIMATE to DONE
void test_logo_screen_animate_to_done(void) {
    LogoScreen logo(0.1f, 1.0f);  // Short wait for testing
    logo.begin(g_test_display, 0x0000);

    // Skip wait phase
    logo.update(0.2f);
    TEST_ASSERT_EQUAL(LogoScreen::State::ANIMATE, logo.getState());

    // Update partway through animation
    logo.update(0.4f);
    TEST_ASSERT_EQUAL(LogoScreen::State::ANIMATE, logo.getState());

    // Complete animation
    logo.update(1.0f);  // Exceed anim duration
    TEST_ASSERT_EQUAL(LogoScreen::State::DONE, logo.getState());
    TEST_ASSERT_TRUE(logo.isDone());
}

// Test: begin() succeeds
void test_logo_screen_begin(void) {
    LogoScreen logo(2.0f, 1.5f);
    bool result = logo.begin(g_test_display, 0x0000);
    TEST_ASSERT_TRUE(result);
}

// Test: State doesn't change after DONE
void test_logo_screen_done_is_final(void) {
    LogoScreen logo(0.1f, 0.1f);
    logo.begin(g_test_display, 0x0000);

    // Fast-forward to DONE
    logo.update(1.0f);
    TEST_ASSERT_EQUAL(LogoScreen::State::DONE, logo.getState());

    // Further updates should not change state
    logo.update(10.0f);
    TEST_ASSERT_EQUAL(LogoScreen::State::DONE, logo.getState());
}

void process(void) {
    UNITY_BEGIN();
    RUN_TEST(test_logo_screen_initial_state);
    RUN_TEST(test_logo_screen_wait_to_animate);
    RUN_TEST(test_logo_screen_animate_to_done);
    RUN_TEST(test_logo_screen_begin);
    RUN_TEST(test_logo_screen_done_is_final);
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
