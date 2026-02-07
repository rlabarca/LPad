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

// Default brightness level (from vendor initSequence.h)
// 175 = vendor default, 255 = max (eliminates PWM flicker)
#define AMOLED_DEFAULT_BRIGHTNESS  255

// Global hardware objects
static Arduino_DataBus *g_bus = nullptr;
static Arduino_RM67162 *g_gfx = nullptr;
static bool g_initialized = false;

// Canvas support
static Arduino_Canvas *g_selected_canvas = nullptr;

/**
 * @brief Wait for the TE (Tearing Effect) signal to sync with display refresh
 *
 * The RM67162 TE pin signals vertical blanking period. By waiting for the
 * TE signal before frame updates, we eliminate tearing artifacts.
 *
 * The TE pin goes LOW during active display scanning and HIGH during
 * vertical blanking (VSYNC). We wait for a complete LOW->HIGH transition
 * to ensure we're at the start of a fresh blanking period.
 */
static void waitForTeSignal(void) {
    static bool te_pin_configured = false;

    // Configure TE pin as input (only once)
    if (!te_pin_configured) {
        pinMode(LCD_TE, INPUT);
        te_pin_configured = true;
    }

    // Ensure we start from a known state by waiting for the current signal to complete
    // This prevents catching the TE signal mid-cycle

    // First, wait for any current HIGH to finish (if we're in blanking period)
    uint32_t timeout = 0;
    while (digitalRead(LCD_TE) == HIGH && timeout++ < 10000) {
        // Fast polling for precise timing
    }

    // Now wait for the scan period to complete (LOW state)
    timeout = 0;
    while (digitalRead(LCD_TE) == LOW && timeout++ < 10000) {
        // Fast polling for precise timing
    }

    // TE just went HIGH - we're now at the START of vertical blanking period
    // This is the optimal moment to begin DMA transfer
}

/**
 * @brief Send vendor-specific initialization sequence for T-Display S3 AMOLED Plus
 *
 * The Arduino_RM67162 driver uses a generic initialization, but this hardware
 * requires additional vendor-specific page register configuration.
 */
static void applyVendorInitSequence() {
    if (!g_bus) return;

    // Vendor-specific initialization sequence (from LilyGo-AMOLED-Series)
    // These page register writes are required for proper operation
    g_bus->beginWrite();

    // Page register configuration
    g_bus->writeC8D8(0xFE, 0x04);  // SET PAGE 3
    g_bus->writeC8D8(0x6A, 0x00);
    g_bus->writeC8D8(0xFE, 0x05);  // SET PAGE 4
    g_bus->writeC8D8(0xFE, 0x07);  // SET PAGE 6
    g_bus->writeC8D8(0x07, 0x4F);
    g_bus->writeC8D8(0xFE, 0x01);  // SET PAGE 0
    g_bus->writeC8D8(0x2A, 0x02);
    g_bus->writeC8D8(0x2B, 0x00);  // Changed from 0x73 to 0x00 to fix Y-offset
    g_bus->writeC8D8(0xFE, 0x0A);  // SET PAGE 9
    g_bus->writeC8D8(0x29, 0x10);
    g_bus->writeC8D8(0xFE, 0x00);  // SET PAGE 0

    // Display control
    g_bus->writeC8D8(0x51, AMOLED_DEFAULT_BRIGHTNESS);  // Write Display Brightness
    g_bus->writeC8D8(0x53, 0x20);  // Write CTRL Display
    g_bus->writeC8D8(0x35, 0x00);  // Tearing Effect Line ON
    g_bus->writeC8D8(0x3A, 0x75);  // Interface Pixel Format (vendor-specific)
    g_bus->writeC8D8(0xC4, 0x80);

    g_bus->endWrite();

    // Delays as per vendor sequence
    delay(120);
}

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
    // Using default FSPI (SPI3) which is the standard for ESP32-S3
    g_bus = new Arduino_ESP32SPI(
        LCD_DC /* DC */,
        LCD_CS /* CS */,
        LCD_SCK /* SCK */,
        LCD_MOSI /* MOSI */,
        GFX_NOT_DEFINED /* MISO */
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

    // Initialize display controller with standard Arduino_RM67162 sequence
    if (!g_gfx->begin(LCD_SPI_FREQ)) {
        return false;  // Display initialization failed
    }

    // Apply vendor-specific initialization required for this hardware
    // Retry twice to prevent initialization failure (per vendor code)
    for (int retry = 0; retry < 2; retry++) {
        applyVendorInitSequence();
    }

    // Explicitly set the address window to cover the full display
    // This ensures we're starting from (0,0) and covering all 240x536 pixels
    g_bus->beginWrite();
    // Column Address Set: 0 to 239
    g_bus->writeCommand(0x2A);  // CASET
    g_bus->write(0x00);  // Start column high byte
    g_bus->write(0x00);  // Start column low byte
    g_bus->write(0x00);  // End column high byte
    g_bus->write(0xEF);  // End column low byte (239)
    // Row Address Set: 0 to 535
    g_bus->writeCommand(0x2B);  // RASET
    g_bus->write(0x00);  // Start row high byte
    g_bus->write(0x00);  // Start row low byte
    g_bus->write(0x02);  // End row high byte
    g_bus->write(0x17);  // End row low byte (535)
    g_bus->endWrite();

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

    // Initialize the canvas, skip parent display reinitialization
    if (!canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) {
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

void* hal_display_get_gfx(void) {
    return static_cast<void*>(g_gfx);
}

void hal_display_fast_blit(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* data) {
    if (!g_initialized || g_gfx == nullptr || data == nullptr) {
        return;
    }

    // Wait for vertical blanking to prevent tearing
    waitForTeSignal();

    // Use Arduino_GFX's optimized bulk transfer method
    // This uses DMA/hardware acceleration instead of pixel-by-pixel loops
    //
    // The sequence is:
    // 1. startWrite() - begin a write transaction
    // 2. writeAddrWindow() - set the rectangular region
    // 3. writePixels() - bulk DMA transfer of the entire buffer
    // 4. endWrite() - end the transaction
    //
    // This is significantly faster than draw16bitRGBBitmap() which loops pixel-by-pixel

    g_gfx->startWrite();
    g_gfx->writeAddrWindow(x, y, w, h);
    g_gfx->writePixels(const_cast<uint16_t*>(data), static_cast<uint32_t>(w) * static_cast<uint32_t>(h));
    g_gfx->endWrite();
}

void hal_display_fast_blit_transparent(int16_t x, int16_t y, int16_t w, int16_t h,
                                       const uint16_t* data, uint16_t transparent_color) {
    if (!g_initialized || g_gfx == nullptr || data == nullptr) {
        return;
    }

    // Wait for vertical blanking to prevent tearing
    waitForTeSignal();

    // Optimized transparent blit using scanline DMA transfers
    // Instead of checking every pixel individually, we:
    // 1. Scan each row to find runs of non-transparent pixels
    // 2. Use DMA (writePixels) to transfer each contiguous run
    // This is much faster than pixel-by-pixel drawing

    g_gfx->startWrite();

    for (int16_t row = 0; row < h; row++) {
        const uint16_t* row_data = data + (row * w);
        int16_t col = 0;

        while (col < w) {
            // Skip transparent pixels
            while (col < w && row_data[col] == transparent_color) {
                col++;
            }

            if (col >= w) break;

            // Find the start of a non-transparent run
            int16_t run_start = col;

            // Find the end of this run
            while (col < w && row_data[col] != transparent_color) {
                col++;
            }

            int16_t run_length = col - run_start;

            // Blit this run using DMA
            if (run_length > 0) {
                g_gfx->writeAddrWindow(x + run_start, y + row, run_length, 1);
                g_gfx->writePixels(const_cast<uint16_t*>(&row_data[run_start]), run_length);
            }
        }
    }

    g_gfx->endWrite();
}

#endif  // !UNIT_TEST
