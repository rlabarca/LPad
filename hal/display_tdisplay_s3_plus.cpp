/**
 * @file display_tdisplay_s3_plus.cpp
 * @brief T-Display-S3 AMOLED Plus Display HAL Implementation
 *
 * This implementation is ported from the vendor examples at:
 * hw-examples/LilyGo-AMOLED-Series/
 *
 * Hardware:
 * - Display Controller: RM67162 (536x240 AMOLED, 1.91 inch)
 * - Communication: QSPI (Quad SPI)
 * - Touch Controller: CST816T (optional)
 */

#ifndef UNIT_TEST  // Only compile for target hardware

#include "display.h"
#include <Arduino.h>
#include <driver/spi_master.h>

// Pin definitions (from BOARD_AMOLED_191 configuration)
#define LCD_D0          18
#define LCD_D1          7
#define LCD_D2          48
#define LCD_D3          5
#define LCD_SCK         47
#define LCD_CS          6
#define LCD_RST         17
#define LCD_TE          9

// Display dimensions (RM67162)
#define LCD_WIDTH       536
#define LCD_HEIGHT      240

// LCD commands
#define LCD_CMD_CASET   0x2A  // Set column address
#define LCD_CMD_RASET   0x2B  // Set row address
#define LCD_CMD_RAMWR   0x2C  // Write frame memory

// SPI configuration
#define DEFAULT_SPI_HANDLER  SPI3_HOST
#define DEFAULT_SCK_SPEED    75000000  // 75 MHz for RM67162
#define SEND_BUF_SIZE        16384
#define TFT_SPI_MODE         SPI_MODE0

// Global hardware objects
static spi_device_handle_t g_spi = nullptr;
static bool g_initialized = false;
static uint16_t g_offset_x = 0;
static uint16_t g_offset_y = 0;

// Command bit and address bit configuration (from vendor RM67162_AMOLED)
static const uint8_t CMD_BIT = 8;
static const uint8_t ADDR_BIT = 24;

// RM67162 initialization sequence (from vendor initSequence.cpp)
typedef struct {
    uint32_t addr;
    uint8_t param[128];
    uint8_t len;
} lcd_cmd_t;

static const lcd_cmd_t rm67162_init_sequence[] = {
    {0x11, {0x00}, 0x80},  // Sleep Out (+ 120ms delay)
    {0x44, {0x01, 0x66}, 0x02},  // Set Tear Scanline
    {0x35, {0x00}, 0x01},  // Tearing Effect Line ON
    {0x53, {0x20}, 0x01},  // Write CTRL Display
    {0x51, {0x00}, 0x01},  // Write Display Brightness
    {0x29, {0x00}, 0x80},  // Display ON (+ 120ms delay)
};

static const uint32_t RM67162_INIT_SEQUENCE_LENGTH = sizeof(rm67162_init_sequence) / sizeof(lcd_cmd_t);

/**
 * @brief Write a command to the display
 *
 * @param cmd Command address
 * @param pdat Parameter data
 * @param length Parameter length
 */
static void writeCommand(uint32_t cmd, uint8_t *pdat, uint32_t length) {
    if (!g_spi) return;

    spi_transaction_ext_t t;
    memset(&t, 0, sizeof(t));

    t.base.flags = SPI_TRANS_MODE_QIO;
    t.base.cmd = 0x02;  // Command write mode
    t.base.addr = cmd << 8;

    if (length > 0 && pdat) {
        t.base.tx_buffer = pdat;
        t.base.length = length * 8;  // Length in bits
    }

    spi_device_polling_transmit(g_spi, (spi_transaction_t *)&t);
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

    bool first_send = true;
    uint16_t *p = data;

    do {
        size_t chunk_size = len;
        if (chunk_size > SEND_BUF_SIZE) {
            chunk_size = SEND_BUF_SIZE;
        }

        spi_transaction_ext_t t;
        memset(&t, 0, sizeof(t));

        if (first_send) {
            t.base.flags = SPI_TRANS_MODE_QIO;
            t.base.cmd = 0x32;  // Memory write continue
            t.base.addr = 0x002C00;
            first_send = false;
        } else {
            t.base.flags = SPI_TRANS_MODE_QIO | SPI_TRANS_VARIABLE_CMD |
                          SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY;
            t.command_bits = 0;
            t.address_bits = 0;
            t.dummy_bits = 0;
        }

        t.base.tx_buffer = p;
        t.base.length = chunk_size * 16;  // Length in bits

        spi_device_polling_transmit(g_spi, (spi_transaction_t *)&t);

        len -= chunk_size;
        p += chunk_size;
    } while (len > 0);

    // Chip select HIGH
    digitalWrite(LCD_CS, HIGH);
}

bool hal_display_init(void) {
    if (g_initialized) {
        return true;  // Already initialized
    }

    // Configure GPIO pins
    pinMode(LCD_RST, OUTPUT);
    pinMode(LCD_CS, OUTPUT);
    pinMode(LCD_TE, INPUT);

    // Reset display
    digitalWrite(LCD_RST, HIGH);
    delay(200);
    digitalWrite(LCD_RST, LOW);
    delay(300);
    digitalWrite(LCD_RST, HIGH);
    delay(200);

    // Configure QSPI bus
    spi_bus_config_t buscfg = {};
    buscfg.data0_io_num = LCD_D0;
    buscfg.data1_io_num = LCD_D1;
    buscfg.sclk_io_num = LCD_SCK;
    buscfg.data2_io_num = LCD_D2;
    buscfg.data3_io_num = LCD_D3;
    buscfg.data4_io_num = -1;
    buscfg.data5_io_num = -1;
    buscfg.data6_io_num = -1;
    buscfg.data7_io_num = -1;
    buscfg.max_transfer_sz = (SEND_BUF_SIZE * 16) + 8;
    buscfg.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS;

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(DEFAULT_SPI_HANDLER, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        return false;  // SPI bus initialization failed
    }

    // Configure SPI device
    spi_device_interface_config_t devcfg = {};
    devcfg.command_bits = CMD_BIT;
    devcfg.address_bits = ADDR_BIT;
    devcfg.mode = TFT_SPI_MODE;
    devcfg.clock_speed_hz = DEFAULT_SCK_SPEED;
    devcfg.spics_io_num = -1;  // CS controlled manually
    devcfg.flags = SPI_DEVICE_HALFDUPLEX;
    devcfg.queue_size = 17;

    ret = spi_bus_add_device(DEFAULT_SPI_HANDLER, &devcfg, &g_spi);
    if (ret != ESP_OK) {
        return false;  // SPI device add failed
    }

    // Send initialization sequence (retry twice to prevent initialization failure)
    for (int retry = 0; retry < 2; retry++) {
        for (uint32_t i = 0; i < RM67162_INIT_SEQUENCE_LENGTH; i++) {
            const lcd_cmd_t *cmd = &rm67162_init_sequence[i];
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
    // The RM67162 via QSPI writes directly to the display without buffering,
    // so flush is a no-op for this hardware.
    // This function exists to satisfy the HAL contract.
}

#endif  // !UNIT_TEST
