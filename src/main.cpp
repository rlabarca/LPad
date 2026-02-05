/**
 * @file main.cpp
 * @brief LPad Main Application Entry Point
 *
 * This is the production entry point for the LPad application.
 * For demo and testing purposes, see the dedicated demo files in the demos/ directory.
 */

#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("=== LPad Application ===");
    Serial.println("Production application not yet implemented.");
    Serial.println("To run demos, use the dedicated build environments:");
    Serial.println("  - demo_screen_esp32s3: Base UI demo for ESP32-S3 AMOLED");
    Serial.println("  - demo_screen_tdisplay: Base UI demo for TDisplay S3 Plus");
}

void loop() {
    delay(1000);
}
