/**
 * @file demo_minimal.cpp
 * @brief Minimal test to verify basic serial output and display initialization
 */

#include <Arduino.h>
#include "display.h"

void setup() {
    Serial.begin(115200);
    delay(2000);  // Longer delay to ensure serial is ready

    Serial.println("\n\n=== MINIMAL DEMO START ===");
    Serial.println("Serial output is working!");
    Serial.flush();

    Serial.println("Attempting display init...");
    Serial.flush();

    if (!hal_display_init()) {
        Serial.println("ERROR: Display init failed");
        Serial.flush();
        while (1) {
            delay(1000);
            Serial.println("Stuck in error loop");
            Serial.flush();
        }
    }

    Serial.println("SUCCESS: Display initialized");
    Serial.flush();

    // Clear screen to red to show display is working
    hal_display_clear(0xF800);  // Red in RGB565
    hal_display_flush();

    Serial.println("Screen should be RED");
    Serial.flush();

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();
    Serial.printf("Display size: %d x %d\n", width, height);
    Serial.flush();

    Serial.println("=== MINIMAL DEMO COMPLETE ===");
    Serial.flush();
}

void loop() {
    delay(1000);
    Serial.println("Loop running...");
    Serial.flush();
}
