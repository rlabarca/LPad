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

// Canvas support
static Arduino_Canvas *g_selected_canvas = nullptr;

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

    // Draw to selected canvas if one is active, otherwise draw to main display
    if (g_selected_canvas != nullptr) {
        g_selected_canvas->fillScreen(color);
    } else {
        g_gfx->fillScreen(color);
    }
}

void hal_display_draw_pixel(int32_t x, int32_t y, uint16_t color) {
    if (!g_initialized || g_gfx == nullptr) {
        return;  // Not initialized
    }

    // Draw to selected canvas if one is active, otherwise draw to main display
    Arduino_GFX *target = (g_selected_canvas != nullptr)
        ? static_cast<Arduino_GFX*>(g_selected_canvas)
        : static_cast<Arduino_GFX*>(g_gfx);

    // Get current logical dimensions (accounts for rotation)
    int32_t width = target->width();
    int32_t height = target->height();

    // Check bounds using logical dimensions
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return;  // Out of bounds, handle gracefully
    }

    target->drawPixel(x, y, color);
}

void hal_display_flush(void) {
    // The Arduino_GFX library for SH8601 writes directly to the display
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

// Canvas-based (Layered) Drawing Implementation

hal_canvas_handle_t hal_display_canvas_create(int16_t width, int16_t height) {
    if (!g_initialized || g_gfx == nullptr) {
        return nullptr;  // Display not initialized
    }

    // Create a new Arduino_Canvas with the specified dimensions
    // Arduino_Canvas uses the parent's bus for actual drawing operations
    Arduino_Canvas *canvas = new Arduino_Canvas(width, height, g_gfx);
    if (!canvas) {
        return nullptr;  // Memory allocation failed
    }

    // Initialize the canvas
    if (!canvas->begin()) {
        delete canvas;
        return nullptr;  // Canvas initialization failed
    }

    return static_cast<hal_canvas_handle_t>(canvas);
}

void hal_display_canvas_delete(hal_canvas_handle_t canvas) {
    if (canvas == nullptr) {
        return;
    }

    // If this canvas is currently selected, deselect it
    Arduino_Canvas *canvas_ptr = static_cast<Arduino_Canvas*>(canvas);
    if (g_selected_canvas == canvas_ptr) {
        g_selected_canvas = nullptr;
    }

    delete canvas_ptr;
}

void hal_display_canvas_select(hal_canvas_handle_t canvas) {
    // Set the selected canvas (nullptr means main display)
    g_selected_canvas = static_cast<Arduino_Canvas*>(canvas);
}

void hal_display_canvas_draw(hal_canvas_handle_t canvas, int32_t x, int32_t y) {
    if (!g_initialized || g_gfx == nullptr || canvas == nullptr) {
        return;
    }

    Arduino_Canvas *canvas_ptr = static_cast<Arduino_Canvas*>(canvas);

    // Use Arduino_GFX's draw16bitRGBBitmap to blit the canvas to the display
    // Get the canvas buffer and dimensions
    uint16_t *buffer = canvas_ptr->getFramebuffer();
    int16_t width = canvas_ptr->width();
    int16_t height = canvas_ptr->height();

    if (buffer != nullptr) {
        g_gfx->draw16bitRGBBitmap(x, y, buffer, width, height);
    }
}

void hal_display_canvas_fill(hal_canvas_handle_t canvas, uint16_t color) {
    if (canvas == nullptr) {
        return;
    }

    Arduino_Canvas *canvas_ptr = static_cast<Arduino_Canvas*>(canvas);
    canvas_ptr->fillScreen(color);
}

#endif  // !UNIT_TEST
