/**
 * @file touch_ft3168.cpp
 * @brief FT3168 Touch Controller HAL Implementation
 *
 * Implements the touch HAL for the Waveshare ESP32-S3 1.8" AMOLED Touch
 * board using the FT3168 touch controller via direct I2C register access.
 *
 * Hardware: Waveshare ESP32-S3-Touch-AMOLED-1.8"
 * Touch controller: FT3168 (FocalTech FT3x68 family)
 * Display: 368x448, portrait orientation (0° rotation)
 * I2C: SDA=GPIO15, SCL=GPIO14, INT=GPIO21, Address=0x38
 */

#ifndef UNIT_TEST  // Only compile for target hardware

#include "touch.h"
#include "input/touch_gesture_engine.h"
#include <Arduino.h>
#include <Wire.h>

// FT3168 pin configuration (Waveshare ESP32-S3 1.8" AMOLED)
#define TOUCH_SDA   15
#define TOUCH_SCL   14
#define TOUCH_INT   21
#define TOUCH_ADDR  0x38

// FT3168 register addresses (FocalTech standard)
#define FT_REG_NUM_TOUCHES  0x02
#define FT_REG_TOUCH1_XH   0x03
#define FT_REG_TOUCH1_XL   0x04
#define FT_REG_TOUCH1_YH   0x05
#define FT_REG_TOUCH1_YL   0x06
#define FT_REG_CHIP_ID     0xA3

static bool g_touch_initialized = false;
static int16_t g_display_width = 0;
static int16_t g_display_height = 0;

// Forward declaration of display dimension getters from display HAL
extern "C" {
    int32_t hal_display_get_width_pixels(void);
    int32_t hal_display_get_height_pixels(void);
}

static bool ft_read_registers(uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    if (Wire.requestFrom((uint8_t)TOUCH_ADDR, len) != len) {
        return false;
    }
    for (uint8_t i = 0; i < len; i++) {
        buf[i] = Wire.read();
    }
    return true;
}

bool hal_touch_init(void) {
    if (g_touch_initialized) {
        return true;
    }

    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(100000);
    Serial.println("[HAL Touch FT3168] I2C bus initialized");

    // Probe the touch controller
    Wire.beginTransmission(TOUCH_ADDR);
    uint8_t err = Wire.endTransmission();
    if (err != 0) {
        Serial.printf("[HAL Touch FT3168] Controller not found at 0x%02X (error %d)\n",
                      TOUCH_ADDR, err);
        return false;
    }

    // Read chip ID for diagnostics
    uint8_t chip_id = 0;
    if (ft_read_registers(FT_REG_CHIP_ID, &chip_id, 1)) {
        Serial.printf("[HAL Touch FT3168] Chip ID: 0x%02X\n", chip_id);
    }

    g_display_width = static_cast<int16_t>(hal_display_get_width_pixels());
    g_display_height = static_cast<int16_t>(hal_display_get_height_pixels());
    Serial.printf("[HAL Touch FT3168] Display: %dx%d\n", g_display_width, g_display_height);

    g_touch_initialized = true;
    Serial.println("[HAL Touch FT3168] Initialized successfully");
    return true;
}

bool hal_touch_read(hal_touch_point_t* point) {
    if (!g_touch_initialized || !point) {
        return false;
    }

    // Read touch registers 0x02-0x06 (5 bytes: num_touches + X/Y coords)
    // FT3168 has no virtual home button
    point->is_home_button = false;

    uint8_t buf[5];
    if (!ft_read_registers(FT_REG_NUM_TOUCHES, buf, 5)) {
        point->is_pressed = false;
        point->x = 0;
        point->y = 0;
        return true;
    }

    uint8_t num_points = buf[0] & 0x0F;
    if (num_points == 0 || num_points > 2) {
        point->is_pressed = false;
        point->x = 0;
        point->y = 0;
        return true;
    }

    // Extract coordinates from FocalTech register format
    int16_t raw_x = ((buf[1] & 0x0F) << 8) | buf[2];
    int16_t raw_y = ((buf[3] & 0x0F) << 8) | buf[4];

    // Debug logging on coordinate change
    static int16_t last_x = -1, last_y = -1;
    if (raw_x != last_x || raw_y != last_y) {
        Serial.printf("[HAL Touch FT3168] RAW: x=%d, y=%d\n", raw_x, raw_y);
        last_x = raw_x;
        last_y = raw_y;
    }

    // Portrait mode (0° rotation), no coordinate transform needed
    // Clamp to display bounds
    int16_t tx = raw_x;
    int16_t ty = raw_y;
    if (tx < 0) tx = 0;
    if (tx >= g_display_width) tx = g_display_width - 1;
    if (ty < 0) ty = 0;
    if (ty >= g_display_height) ty = g_display_height - 1;

    point->x = tx;
    point->y = ty;
    point->is_pressed = true;

    return true;
}

void hal_touch_configure_gesture_engine(TouchGestureEngine* engine) {
    if (!engine) return;
    // Waveshare 368x448 portrait - default percentage-based edge zones work well
    // No special configuration needed for full-screen touch panel
}

#endif // !UNIT_TEST
