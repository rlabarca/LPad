/**
 * @file demo_screen.cpp
 * @brief Release 0.5 Demo Application Entry Point
 *
 * This file serves as the main entry point for the demo_v05_* build environments.
 * It uses the V05DemoApp class which implements the full v0.5 demo specification:
 * - Logo Animation
 * - 6 Graph Modes (2 layouts × 3 themes)
 *
 * See features/demo_release_0.5.md for specification.
 */

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "v05_demo_app.h"
#include "display.h"
#include "relative_display.h"
#include "animation_ticker.h"

// Global instances
V05DemoApp* g_demoApp = nullptr;
AnimationTicker* g_ticker = nullptr;
RelativeDisplay* g_relativeDisplay = nullptr;

void displayError(const char* message) {
    hal_display_clear(RGB565_RED);
    hal_display_flush();
    Serial.println("=== ERROR ===");
    Serial.println(message);
    Serial.println("=============");
}

void setup() {
    Serial.begin(115200);
    delay(500);  // Brief delay for ESP32-S3 USB CDC
    yield();

    Serial.println("\n\n\n=== LPad Release 0.5 Demo Application ===");
    Serial.println("Using V05DemoApp class");
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

    // [4/4] Create V05DemoApp
    Serial.println("[4/4] Creating V05DemoApp...");
    Serial.flush();

    static V05DemoApp demoApp;
    g_demoApp = &demoApp;

    if (!g_demoApp->begin(g_relativeDisplay)) {
        displayError("V05DemoApp initialization failed");
        while (1) delay(1000);
    }

    Serial.println("  [PASS] V05DemoApp initialized");
    Serial.println();

    Serial.println("=== Release 0.5 Demo Application Ready ===");
    Serial.println();
    Serial.println("Demo Cycle (loops indefinitely):");
    Serial.println("  1. Logo Animation (wait 2s + animate 1.5s + hold 2s)");
    Serial.println("  2. Graph Mode 0: Scientific + Gradient (5s)");
    Serial.println("  3. Graph Mode 1: Scientific + Solid (5s)");
    Serial.println("  4. Graph Mode 2: Scientific + Mixed (5s)");
    Serial.println("  5. Graph Mode 3: Compact + Gradient (5s)");
    Serial.println("  6. Graph Mode 4: Compact + Solid (5s)");
    Serial.println("  7. Graph Mode 5: Compact + Mixed (5s)");
    Serial.println("  8. Return to step 1");
    Serial.println();
    Serial.println("Starting animation loop...");
    Serial.println();
}

void loop() {
    // Wait for next frame and get deltaTime
    float deltaTime = g_ticker->waitForNextFrame();

    // Update and render demo app
    if (g_demoApp != nullptr) {
        g_demoApp->update(deltaTime);
        g_demoApp->render();

        // Check if cycle finished, restart from logo
        if (g_demoApp->isFinished()) {
            Serial.println("\n=== Demo cycle finished, restarting from logo ===\n");
            delete g_demoApp;
            g_demoApp = new V05DemoApp();
            if (!g_demoApp->begin(g_relativeDisplay)) {
                displayError("V05DemoApp re-initialization failed");
                while (1) delay(1000);
            }
        }
    }
}
