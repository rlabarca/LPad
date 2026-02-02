/**
 * @file display_tdisplay_s3_plus.cpp
 * @brief T-Display-S3 AMOLED Plus Display HAL Implementation
 *
 * This implementation is ported from the vendor examples at:
 * hw-examples/LilyGo-AMOLED-Series/
 *
 * Hardware:
 * - Display Controller: RM67162 (240x536 AMOLED, 1.91 inch)
 * - Communication: SPI (NOT QSPI - this is the Plus model)
 * - Touch Controller: CST816T (optional)
 */

#ifndef UNIT_TEST  // Only compile for target hardware

#include "display.h"
#include <Arduino.h>
#include <SPI.h>

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

// LCD commands
#define LCD_CMD_CASET   0x2A  // Set column address
#define LCD_CMD_RASET   0x2B  // Set row address
#define LCD_CMD_RAMWR   0x2C  // Write frame memory
#define LCD_CMD_MADCTL  0x36  // Memory Data Access Control

// MADCTL bits (RM67162)
#define MADCTL_MY       0x80  // Row Address Order
#define MADCTL_MX       0x40  // Column Address Order
#define MADCTL_MV       0x20  // Row/Column Exchange
#define MADCTL_ML       0x10  // Vertical Refresh Order
#define MADCTL_RGB      0x00  // RGB Order
#define MADCTL_BGR      0x08  // BGR Order

// SPI configuration
#define LCD_SPI_FREQ    40000000  // 40 MHz for RM67162 SPI mode

// Default brightness level (from vendor initSequence.h)
#define AMOLED_DEFAULT_BRIGHTNESS  175

// Global hardware objects
static SPIClass *g_spi = nullptr;
static bool g_initialized = false;
static uint16_t g_offset_x = 0;
static uint16_t g_offset_y = 0;
static int g_rotation = 0;  // Current rotation in degrees

// RM67162 SPI initialization sequence (from vendor initSequence.cpp)
typedef struct {
    uint32_t addr;
    uint8_t param[20];
    uint8_t len;
} lcd_cmd_t;

static const lcd_cmd_t rm67162_spi_init_sequence[] = {
    {0xFE, {0x04}, 0x01},                               // SET PAGE 3
    {0x6A, {0x00}, 0x01},
    {0xFE, {0x05}, 0x01},                               // SET PAGE 4
    {0xFE, {0x07}, 0x01},                               // SET PAGE 6
    {0x07, {0x4F}, 0x01},
    {0xFE, {0x01}, 0x01},                               // SET PAGE 0
    {0x2A, {0x02}, 0x01},
    {0x2B, {0x73}, 0x01},
    {0xFE, {0x0A}, 0x01},                               // SET PAGE 9
    {0x29, {0x10}, 0x01},
    {0xFE, {0x00}, 0x01},                               // SET PAGE 0
    {0x51, {AMOLED_DEFAULT_BRIGHTNESS}, 0x01},          // Write Display Brightness
    {0x53, {0x20}, 0x01},                               // Write CTRL Display
    {0x35, {0x00}, 0x01},                               // Tearing Effect Line ON
    {0x3A, {0x75}, 0x01},                               // Interface Pixel Format 16bit/pixel
    {0xC4, {0x80}, 0x01},
    {0x11, {0x00}, 0x01 | 0x80},                        // Sleep Out (+ 120ms delay)
    {0x29, {0x00}, 0x01 | 0x80},                        // Display ON (+ 120ms delay)
};

static const uint32_t RM67162_INIT_SPI_SEQUENCE_LENGTH = sizeof(rm67162_spi_init_sequence) / sizeof(lcd_cmd_t);

/**
 * @brief Write a command to the display using SPI
 *
 * @param cmd Command byte
 * @param pdat Parameter data
 * @param length Parameter length
 */
static void writeCommand(uint32_t cmd, uint8_t *pdat, uint32_t length) {
    if (!g_spi) return;

    // Chip select LOW
    digitalWrite(LCD_CS, LOW);

    g_spi->beginTransaction(SPISettings(LCD_SPI_FREQ, MSBFIRST, SPI_MODE0));

    // Send command byte (DC LOW)
    digitalWrite(LCD_DC, LOW);
    g_spi->write(cmd);

    // Send data bytes if present (DC HIGH)
    if (pdat && length > 0) {
        digitalWrite(LCD_DC, HIGH);
        g_spi->writeBytes(pdat, length);
    }

    g_spi->endTransaction();

    // Chip select HIGH
    digitalWrite(LCD_CS, HIGH);
}

/**
 * @brief Set the display address window for drawing
 *
 * @param xs Start X coordinate
 * @param ys Start Y coordinate
 * @param xe End X coordinate
 * @param ye End Y coordinate
 */
static void setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye) {
    xs += g_offset_x;
    ys += g_offset_y;
    xe += g_offset_x;
    ye += g_offset_y;

    // Set column address
    uint8_t caset_params[4] = {
        (uint8_t)((xs >> 8) & 0xFF),
        (uint8_t)(xs & 0xFF),
        (uint8_t)((xe >> 8) & 0xFF),
        (uint8_t)(xe & 0xFF)
    };
    writeCommand(LCD_CMD_CASET, caset_params, 4);

    // Set row address
    uint8_t raset_params[4] = {
        (uint8_t)((ys >> 8) & 0xFF),
        (uint8_t)(ys & 0xFF),
        (uint8_t)((ye >> 8) & 0xFF),
        (uint8_t)(ye & 0xFF)
    };
    writeCommand(LCD_CMD_RASET, raset_params, 4);

    // Start memory write
    writeCommand(LCD_CMD_RAMWR, nullptr, 0);
}

/**
 * @brief Push color data to the display
 *
 * @param data Pointer to 16-bit RGB565 color data
 * @param len Number of pixels
 */
static void pushColors(uint16_t *data, uint32_t len) {
    if (!g_spi || !data) return;

    // Chip select LOW
    digitalWrite(LCD_CS, LOW);

    g_spi->beginTransaction(SPISettings(LCD_SPI_FREQ, MSBFIRST, SPI_MODE0));

    // Data mode (DC HIGH)
    digitalWrite(LCD_DC, HIGH);

    // Write pixel data with correct byte order
    // Send high byte first, then low byte (big-endian over SPI)
    for (uint32_t i = 0; i < len; i++) {
        uint16_t color = data[i];
        uint8_t hi = (color >> 8) & 0xFF;
        uint8_t lo = color & 0xFF;
        g_spi->write(hi);  // Send high byte first
        g_spi->write(lo);  // Send low byte second
    }

    g_spi->endTransaction();

    // Chip select HIGH
    digitalWrite(LCD_CS, HIGH);
}

bool hal_display_init(void) {
    if (g_initialized) {
        return true;  // Already initialized
    }

    // Enable PMIC to power the display
    pinMode(LCD_PMIC_EN, OUTPUT);
    digitalWrite(LCD_PMIC_EN, HIGH);
    delay(10);

    // Configure GPIO pins
    pinMode(LCD_RST, OUTPUT);
    pinMode(LCD_CS, OUTPUT);
    pinMode(LCD_DC, OUTPUT);
    pinMode(LCD_TE, INPUT);

    // Initialize pins to idle state
    digitalWrite(LCD_CS, HIGH);
    digitalWrite(LCD_DC, HIGH);

    // Reset display
    digitalWrite(LCD_RST, HIGH);
    delay(200);
    digitalWrite(LCD_RST, LOW);
    delay(300);
    digitalWrite(LCD_RST, HIGH);
    delay(200);

    // Initialize SPI
    g_spi = new SPIClass(HSPI);
    if (!g_spi) {
        return false;  // Memory allocation failed
    }
    g_spi->begin(LCD_SCK, -1 /*MISO not used*/, LCD_MOSI, LCD_CS);

    // Send initialization sequence (retry twice to prevent initialization failure)
    for (int retry = 0; retry < 2; retry++) {
        for (uint32_t i = 0; i < RM67162_INIT_SPI_SEQUENCE_LENGTH; i++) {
            const lcd_cmd_t *cmd = &rm67162_spi_init_sequence[i];
            writeCommand(cmd->addr, (uint8_t *)cmd->param, cmd->len & 0x1F);

            // Check delay flags
            if (cmd->len & 0x80) {
                delay(120);  // Long delay
            }
            if (cmd->len & 0x20) {
                delay(10);  // Short delay
            }
        }
    }

    g_initialized = true;
    return true;
}

void hal_display_clear(uint16_t color) {
    if (!g_initialized) {
        return;  // Not initialized
    }

    // Set address window to full screen
    setAddrWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    // Allocate buffer for one row
    uint16_t *line_buffer = (uint16_t *)malloc(LCD_WIDTH * sizeof(uint16_t));
    if (!line_buffer) {
        return;  // Memory allocation failed
    }

    // Fill buffer with color
    for (int i = 0; i < LCD_WIDTH; i++) {
        line_buffer[i] = color;
    }

    // Push the same row multiple times to fill the screen
    for (int y = 0; y < LCD_HEIGHT; y++) {
        pushColors(line_buffer, LCD_WIDTH);
    }

    free(line_buffer);
}

void hal_display_draw_pixel(int32_t x, int32_t y, uint16_t color) {
    if (!g_initialized) {
        return;  // Not initialized
    }

    // Check bounds
    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) {
        return;  // Out of bounds, handle gracefully
    }

    // Set address window for single pixel
    setAddrWindow(x, y, x, y);

    // Push single pixel color
    pushColors(&color, 1);
}

void hal_display_flush(void) {
    // The RM67162 via SPI writes directly to the display without buffering,
    // so flush is a no-op for this hardware.
    // This function exists to satisfy the HAL contract.
}

int32_t hal_display_get_width_pixels(void) {
    // Swap dimensions for 90 and 270 degree rotations
    if (g_rotation == 90 || g_rotation == 270) {
        return LCD_HEIGHT;
    }
    return LCD_WIDTH;
}

int32_t hal_display_get_height_pixels(void) {
    // Swap dimensions for 90 and 270 degree rotations
    if (g_rotation == 90 || g_rotation == 270) {
        return LCD_WIDTH;
    }
    return LCD_HEIGHT;
}

void hal_display_set_rotation(int degrees) {
    if (!g_initialized) {
        return;  // Not initialized
    }

    // Store the rotation
    g_rotation = degrees;

    // Determine MADCTL value based on rotation
    // Based on Arduino_RM67162 implementation
    uint8_t madctl_value;
    switch (degrees) {
        case 0:
            madctl_value = MADCTL_RGB;
            break;
        case 90:
            madctl_value = MADCTL_MX | MADCTL_MV | MADCTL_RGB;
            break;
        case 180:
            madctl_value = MADCTL_MX | MADCTL_MY | MADCTL_RGB;
            break;
        case 270:
            madctl_value = MADCTL_MV | MADCTL_MY | MADCTL_RGB;
            break;
        default:
            madctl_value = MADCTL_RGB;  // Default to 0 degrees
            break;
    }

    // Send MADCTL command
    writeCommand(LCD_CMD_MADCTL, &madctl_value, 1);
}

#endif  // !UNIT_TEST
