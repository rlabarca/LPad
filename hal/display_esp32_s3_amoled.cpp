/**
 * @file display_esp32_s3_amoled.cpp
 * @brief ESP32-S3-Touch-AMOLED-1.8 Display HAL Implementation
 *
 * This implementation is ported from the vendor examples at:
 * hw-examples/ESP32-S3-Touch-AMOLED-1.8-Demo/Arduino-v3.3.5/
 *
 * Hardware:
 * - Display Controller: SH8601 (368x448 AMOLED)
 * - Communication: QSPI (Quad SPI)
 * - Power Management: XCA9554 GPIO Expander
 */

#ifndef UNIT_TEST  // Only compile for target hardware

#include "display.h"
#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include "Arduino_DriveBus_Library.h"
#include <Adafruit_XCA9554.h>

// Pin definitions (from vendor pin_config.h)
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 11
#define LCD_CS 12
#define LCD_WIDTH 368
#define LCD_HEIGHT 448

#define IIC_SDA 15
#define IIC_SCL 14

// GPIO Expander power control pins
#define EXPANDER_PIN_POWER_0 0
#define EXPANDER_PIN_POWER_1 1
#define EXPANDER_PIN_POWER_2 2

// I2C address for XCA9554 GPIO expander
#define EXPANDER_I2C_ADDRESS 0x20

// Global hardware objects
static Arduino_DataBus *g_bus = nullptr;
static Arduino_SH8601 *g_gfx = nullptr;
static Adafruit_XCA9554 g_expander;
static bool g_initialized = false;

bool hal_display_init(void) {
    if (g_initialized) {
        return true;  // Already initialized
    }

    // Initialize I2C for power management
    Wire.begin(IIC_SDA, IIC_SCL);

    // Initialize GPIO expander for power control
    if (!g_expander.begin(EXPANDER_I2C_ADDRESS)) {
        return false;  // Failed to find XCA9554 chip
    }

    // Configure expander pins for power management
    g_expander.pinMode(EXPANDER_PIN_POWER_0, OUTPUT);
    g_expander.pinMode(EXPANDER_PIN_POWER_1, OUTPUT);
    g_expander.pinMode(EXPANDER_PIN_POWER_2, OUTPUT);

    // Power sequencing: Start LOW
    g_expander.digitalWrite(EXPANDER_PIN_POWER_0, LOW);
    g_expander.digitalWrite(EXPANDER_PIN_POWER_1, LOW);
    g_expander.digitalWrite(EXPANDER_PIN_POWER_2, LOW);
    delay(20);

    // Power sequencing: Set HIGH to enable display power
    g_expander.digitalWrite(EXPANDER_PIN_POWER_0, HIGH);
    g_expander.digitalWrite(EXPANDER_PIN_POWER_1, HIGH);
    g_expander.digitalWrite(EXPANDER_PIN_POWER_2, HIGH);
    delay(20);

    // Create QSPI bus for display communication
    g_bus = new Arduino_ESP32QSPI(
        LCD_CS /* CS */,
        LCD_SCLK /* SCK */,
        LCD_SDIO0 /* SDIO0 */,
        LCD_SDIO1 /* SDIO1 */,
        LCD_SDIO2 /* SDIO2 */,
        LCD_SDIO3 /* SDIO3 */
    );

    // Create SH8601 display driver
    g_gfx = new Arduino_SH8601(
        g_bus,
        GFX_NOT_DEFINED /* RST */,
        0 /* rotation */,
        LCD_WIDTH /* width */,
        LCD_HEIGHT /* height */
    );

    // Initialize display controller
    if (!g_gfx->begin()) {
        return false;  // Display initialization failed
    }

    // Set maximum brightness
    g_gfx->setBrightness(255);

    g_initialized = true;
    return true;
}

void hal_display_clear(uint16_t color) {
    if (!g_initialized || g_gfx == nullptr) {
        return;  // Not initialized
    }

    g_gfx->fillScreen(color);
}

void hal_display_draw_pixel(int32_t x, int32_t y, uint16_t color) {
    if (!g_initialized || g_gfx == nullptr) {
        return;  // Not initialized
    }

    // Check bounds
    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) {
        return;  // Out of bounds, handle gracefully
    }

    g_gfx->drawPixel(x, y, color);
}

void hal_display_flush(void) {
    // The Arduino_GFX library for SH8601 writes directly to the display
    // without buffering, so flush is a no-op for this hardware.
    // This function exists to satisfy the HAL contract.
}

int32_t hal_display_get_width_pixels(void) {
    return LCD_WIDTH;
}

int32_t hal_display_get_height_pixels(void) {
    return LCD_HEIGHT;
}

#endif  // !UNIT_TEST
