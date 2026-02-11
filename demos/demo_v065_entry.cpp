/**
 * @file demo_v065_entry.cpp
 * @brief Release 0.65 Demo Entry Point Implementation
 */

#include "demo_v065_entry.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "v065_demo_app.h"
#include "../hal/display.h"
#include "../hal/touch.h"
#include "relative_display.h"
#include "animation_ticker.h"

// Global instances
static V065DemoApp* g_demoApp = nullptr;
static AnimationTicker* g_ticker = nullptr;
static RelativeDisplay* g_relativeDisplay = nullptr;

static void displayError(const char* message) {
    hal_display_clear(RGB565_RED);
    hal_display_flush();
    Serial.println("=== ERROR ===");
    Serial.println(message);
    Serial.println("=============");
}

void demo_setup() {
    Serial.begin(115200);
    delay(500);  // Brief delay for ESP32-S3 USB CDC
    yield();

    Serial.println("\n\n\n=== LPad Release 0.65 Demo Application ===");
    Serial.println("Demo Flow: Logo -> WiFi -> Stock Tracker with Touch Interaction");
    Serial.flush();
    yield();

    // [1/5] Initialize display HAL
    Serial.println("[1/5] Initializing display HAL...");
    Serial.flush();

    if (!hal_display_init()) {
        displayError("Display initialization failed");
        while (1) delay(1000);
    }
    Serial.println("  [PASS] Display initialized");

    #ifdef APP_DISPLAY_ROTATION
    Serial.printf("  [INFO] Applying rotation: %d degrees\n", APP_DISPLAY_ROTATION);
    hal_display_set_rotation(APP_DISPLAY_ROTATION);
    #endif

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();
    Serial.printf("  [INFO] Display resolution: %d x %d pixels\n", width, height);
    Serial.println();
    yield();

    // [2/5] Initialize touch HAL
    Serial.println("[2/5] Initializing touch HAL...");
    Serial.flush();

    if (!hal_touch_init()) {
        displayError("Touch initialization failed");
        while (1) delay(1000);
    }
    Serial.println("  [PASS] Touch initialized");
    Serial.println();
    yield();

    // [3/5] Initialize RelativeDisplay API
    Serial.println("[3/5] Initializing RelativeDisplay abstraction...");
    Serial.flush();

    display_relative_init();
    Arduino_GFX* display = static_cast<Arduino_GFX*>(hal_display_get_gfx());
    if (display == nullptr) {
        displayError("Display object unavailable");
        while (1) delay(1000);
    }

    static RelativeDisplay relDisplay(display, width, height);
    g_relativeDisplay = &relDisplay;
    g_relativeDisplay->init();

    Serial.println("  [PASS] RelativeDisplay initialized");
    Serial.println();
    yield();

    // [4/5] Create AnimationTicker
    Serial.println("[4/5] Creating 30fps AnimationTicker...");
    Serial.flush();

    static AnimationTicker ticker(30);
    g_ticker = &ticker;
    Serial.println("  [PASS] AnimationTicker created (30fps)");
    Serial.println();
    yield();

    // [5/5] Create V065DemoApp
    Serial.println("[5/5] Creating V065DemoApp...");
    Serial.flush();

    static V065DemoApp demoApp;
    g_demoApp = &demoApp;

    if (!g_demoApp->begin(g_relativeDisplay)) {
        displayError("V065DemoApp initialization failed");
        while (1) delay(1000);
    }

    Serial.println("  [PASS] V065DemoApp initialized");
    Serial.println();

    Serial.println("=== Demo Started ===");
    Serial.println("Touch the screen to see gesture debug overlay");
    Serial.flush();
}

void demo_loop() {
    if (!g_demoApp || !g_ticker) {
        return;
    }

    // Wait for next frame
    if (!g_ticker->shouldTick()) {
        return;
    }

    float deltaTime = g_ticker->getDeltaTime();

    // Update and render demo
    g_demoApp->update(deltaTime);
    g_demoApp->render();

    g_ticker->markFrameComplete();
}
