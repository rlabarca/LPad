/**
 * @file demo_v060_entry.cpp
 * @brief Release 0.60 Demo Entry Point Implementation
 */

#include "demo_v060_entry.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "v060_demo_app.h"
#include "../hal/display.h"
#include "relative_display.h"
#include "animation_ticker.h"

// Global instances
static V060DemoApp* g_demoApp = nullptr;
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

    Serial.println("\n\n\n=== LPad Release 0.60 Demo Application ===");
    Serial.println("Demo Flow: Logo -> WiFi -> Stock Tracker (^TNX)");
    Serial.flush();
    yield();

    // [1/4] Initialize display HAL
    Serial.println("[1/4] Initializing display HAL...");
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

    // [2/4] Initialize RelativeDisplay API
    Serial.println("[2/4] Initializing RelativeDisplay abstraction...");
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

    // [3/4] Create AnimationTicker
    Serial.println("[3/4] Creating 30fps AnimationTicker...");
    Serial.flush();

    static AnimationTicker ticker(30);
    g_ticker = &ticker;
    Serial.println("  [PASS] AnimationTicker created (30fps)");
    Serial.println();
    yield();

    // [4/4] Create V060DemoApp
    Serial.println("[4/4] Creating V060DemoApp...");
    Serial.flush();

    static V060DemoApp demoApp;
    g_demoApp = &demoApp;

    if (!g_demoApp->begin(g_relativeDisplay)) {
        displayError("V060DemoApp initialization failed");
        while (1) delay(1000);
    }

    Serial.println("  [PASS] V060DemoApp initialized");
    Serial.println();

    Serial.println("=== Release 0.60 Demo Application Ready ===");
    Serial.println();
    Serial.println("Demo Flow:");
    Serial.println("  1. Logo Animation (wait 2s + animate + hold 2s)");
    Serial.println("  2. WiFi Connectivity Check + Ping Test");
    Serial.println("  3. Hold 'PING OK' for 2 seconds");
    Serial.println("  4. Stock Tracker Graph (^TNX with live updates)");
    Serial.println();
    Serial.println("Starting animation loop...");
    Serial.println();
}

void demo_loop() {
    // Wait for next frame and get deltaTime
    float deltaTime = g_ticker->waitForNextFrame();

    // Update and render demo app
    if (g_demoApp != nullptr) {
        g_demoApp->update(deltaTime);
        g_demoApp->render();
    }
}
