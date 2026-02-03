/**
 * @file display_tdisplay_s3_plus.cpp
 * @brief T-Display-S3 AMOLED Plus Display HAL Implementation (Arduino_GFX)
 *
 * This implementation is ported from the vendor examples at:
 * hw-examples/ESP32-S3-Touch-AMOLED-1.8-Demo/Arduino-v3.3.5/
 *
 * Hardware:
 * - Display Controller: RM67162 (240x536 AMOLED, 1.91 inch)
 * - Communication: SPI (NOT QSPI - this is the Plus model)
 * - Touch Controller: CST816T (optional)
 */

#ifndef UNIT_TEST  // Only compile for target hardware

#include "display.h"
#include <Arduino.h>
#include "Arduino_GFX_Library.h"

// Pin definitions (from BOARD_AMOLED_191_SPI configuration)
#define LCD_MOSI        18
#define LCD_DC          7   // Data/Command pin (critical for SPI)
#define LCD_SCK         47
#define LCD_CS          6
#define LCD_RST         17
#define LCD_TE          9
#define LCD_PMIC_EN     38  // PMIC enable pin

// Display dimensions (RM67162)
#define LCD_WIDTH       240
#define LCD_HEIGHT      536

// SPI configuration
#define LCD_SPI_FREQ    40000000  // 40 MHz for RM67162 SPI mode

// Global hardware objects
static Arduino_DataBus *g_bus = nullptr;
static Arduino_RM67162 *g_gfx = nullptr;
static bool g_initialized = false;

bool hal_display_init(void) {
    if (g_initialized) {
        return true;  // Already initialized
    }

    // Enable PMIC to power the display
    pinMode(LCD_PMIC_EN, OUTPUT);
    digitalWrite(LCD_PMIC_EN, HIGH);
    delay(10);

    // Create SPI bus for display communication
    // Arduino_ESP32SPI(dc, cs, sck, mosi, miso, spi_num, is_shared_interface)
    // Note: Using HSPI (SPI2) to match the original implementation
    g_bus = new Arduino_ESP32SPI(
        LCD_DC /* DC */,
        LCD_CS /* CS */,
        LCD_SCK /* SCK */,
        LCD_MOSI /* MOSI */,
        GFX_NOT_DEFINED /* MISO */,
        HSPI /* spi_num */,
        true /* is_shared_interface */
    );

    if (!g_bus) {
        return false;  // Memory allocation failed
    }

    // Create RM67162 display driver
    // Arduino_RM67162(bus, rst, rotation, ips)
    g_gfx = new Arduino_RM67162(
        g_bus,
        LCD_RST /* RST */,
        0 /* rotation */,
        false /* ips */
    );

    if (!g_gfx) {
        return false;  // Memory allocation failed
    }

    // Initialize display controller
    if (!g_gfx->begin(LCD_SPI_FREQ)) {
        return false;  // Display initialization failed
    }

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
    // The Arduino_GFX library for RM67162 writes directly to the display
    // without buffering, so flush is a no-op for this hardware.
    // This function exists to satisfy the HAL contract.
}

int32_t hal_display_get_width_pixels(void) {
    if (g_initialized && g_gfx != nullptr) {
        return g_gfx->width();
    }
    return LCD_WIDTH;
}

int32_t hal_display_get_height_pixels(void) {
    if (g_initialized && g_gfx != nullptr) {
        return g_gfx->height();
    }
    return LCD_HEIGHT;
}

void hal_display_set_rotation(int degrees) {
    if (!g_initialized || g_gfx == nullptr) {
        return;  // Not initialized
    }

    // Arduino_GFX uses rotation index (0-3) instead of degrees
    // 0 = 0°, 1 = 90°, 2 = 180°, 3 = 270°
    uint8_t rotation_index = 0;
    switch (degrees) {
        case 0:   rotation_index = 0; break;
        case 90:  rotation_index = 1; break;
        case 180: rotation_index = 2; break;
        case 270: rotation_index = 3; break;
        default:  rotation_index = 0; break;  // Default to 0 for invalid values
    }

    g_gfx->setRotation(rotation_index);
}

#endif  // !UNIT_TEST
