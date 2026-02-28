/**
 * @file touch_cst816.cpp
 * @brief CST816 Touch Controller HAL Implementation (Direct I2C)
 *
 * Implements the touch HAL for the T-Display S3 AMOLED Plus (1.91")
 * using direct I2C register reads, bypassing SensorLib.
 *
 * This approach was adopted because SensorLib's TouchDrvCSTXXX wrapper
 * was only returning the home button coordinate (600, 120) and never
 * real touch points — likely due to auto-sleep mode or initialization
 * issues that could not be debugged through the library abstraction.
 *
 * Direct I2C approach proven reliable with FT3168 HAL.
 *
 * Specification: features/touch_cst816_implementation.md
 */

#ifndef UNIT_TEST  // Only compile for target hardware

#include "touch.h"
#include "input/touch_gesture_engine.h"
#include <Arduino.h>
#include <Wire.h>

// Board-specific pin configuration
#if defined(APP_DISPLAY_ROTATION)
    // T-Display S3 AMOLED Plus (1.91")
    #define TOUCH_SDA 3
    #define TOUCH_SCL 2
    #define TOUCH_INT 21
    #define TOUCH_ADDR 0x15
    #define DISPLAY_ROTATION 90  // Landscape
#else
    // ESP32-S3-Touch-AMOLED (1.8")
    #define TOUCH_SDA 15
    #define TOUCH_SCL 14
    #define TOUCH_INT 21
    #define TOUCH_ADDR 0x15
    #define DISPLAY_ROTATION 0  // Portrait
#endif

// CST816 register addresses
#define CST_REG_STATUS        0x00  // Read 13 bytes for full touch data
#define CST_REG_TOUCH_COUNT   0x02  // Number of touch points (lower 4 bits)
#define CST_REG_XPOS_HIGH     0x03  // X position high byte (lower 4 bits)
#define CST_REG_XPOS_LOW      0x04  // X position low byte
#define CST_REG_YPOS_HIGH     0x05  // Y position high byte (lower 4 bits)
#define CST_REG_YPOS_LOW      0x06  // Y position low byte
#define CST_REG_CHIP_ID       0xA7  // Chip ID register
#define CST_REG_FW_VERSION    0xA9  // Firmware version
#define CST_REG_SLEEP         0xE5  // Sleep control (write 0x03 to sleep)
#define CST_REG_DIS_AUTOSLEEP 0xFE  // Disable auto-sleep (write 0x01)

// Expected chip IDs
#define CST816S_CHIP_ID  0xB4
#define CST816T_CHIP_ID  0xB5
#define CST816D_CHIP_ID  0xB6
#define CST820_CHIP_ID   0xB7

// Home button coordinate (hardware-reported, must be filtered)
#define HOME_BTN_X 600
#define HOME_BTN_Y 120

static bool g_touch_initialized = false;
static int16_t g_display_width = 0;
static int16_t g_display_height = 0;

// Forward declaration of display dimension getters from display HAL
extern "C" {
    int32_t hal_display_get_width_pixels(void);
    int32_t hal_display_get_height_pixels(void);
}

// Direct I2C register read with retry (same pattern as FT3168 HAL)
// Single retry suppresses transient NACK errors caused by WiFi interrupt timing
static bool cst_read_registers(uint8_t reg, uint8_t* buf, uint8_t len) {
    for (int attempt = 0; attempt < 2; attempt++) {
        Wire.beginTransmission(TOUCH_ADDR);
        Wire.write(reg);
        if (Wire.endTransmission(false) != 0) {
            continue;
        }
        if (Wire.requestFrom((uint8_t)TOUCH_ADDR, len) != len) {
            continue;
        }
        for (uint8_t i = 0; i < len; i++) {
            buf[i] = Wire.read();
        }
        return true;
    }
    return false;
}

// Direct I2C single-byte write
static bool cst_write_register(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(TOUCH_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool hal_touch_init(void) {
    if (g_touch_initialized) {
        return true;
    }

    // Quick probe: if auto-sleep was kept disabled (e.g., after light sleep
    // resume), the CST816 is still active and will respond immediately.
    // This avoids the slow INT toggle retry loop in the common wake path.
    Wire.end();
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire.setClock(100000);

    Wire.beginTransmission(TOUCH_ADDR);
    bool found = (Wire.endTransmission() == 0);
    if (found) {
        Serial.println("[HAL Touch CST816] Quick probe: ACK (chip already active)");
    } else {
        // Chip is in auto-sleep or fresh from power-on. Use INT pin toggle
        // (pseudo-reset) with retry. Without a dedicated RST pin, driving
        // INT low may force the controller out of auto-sleep/gesture-only
        // mode on some chip variants. Retry with progressively longer pulses
        // AND post-release delays.
        //
        // Wire.end() is called before each attempt because Wire.begin()'s
        // internal _started flag survives across calls — without end(), the
        // I2C peripheral is NOT reinitialized after a failed probe, leaving
        // stale driver state that blocks all subsequent attempts.
        for (int attempt = 0; attempt < 5 && !found; attempt++) {
            int pulse_ms = 50 + attempt * 50;   // 50, 100, 150, 200, 250ms
            int settle_ms = 100 + attempt * 50;  // 100, 150, 200, 250, 300ms
            Serial.printf("[HAL Touch CST816] Wake attempt %d (INT LOW %dms, settle %dms)...\n",
                          attempt + 1, pulse_ms, settle_ms);

            // Reset I2C peripheral to ensure clean state between attempts
            Wire.end();

            pinMode(TOUCH_INT, OUTPUT);
            digitalWrite(TOUCH_INT, LOW);
            delay(pulse_ms);
            pinMode(TOUCH_INT, INPUT);
            delay(settle_ms);

            Wire.begin(TOUCH_SDA, TOUCH_SCL);
            Wire.setClock(100000);

            Wire.beginTransmission(TOUCH_ADDR);
            uint8_t err = Wire.endTransmission();
            if (err == 0) {
                found = true;
            } else {
                Serial.printf("[HAL Touch CST816] Attempt %d: no ACK (error %d)\n",
                              attempt + 1, err);
            }
        }
    }

    if (!found) {
        Serial.printf("[HAL Touch CST816] Controller not found at 0x%02X after 5 attempts\n",
                      TOUCH_ADDR);
        return false;
    }
    Serial.println("[HAL Touch CST816] Controller found on I2C bus");

    // Read chip ID
    uint8_t chip_id = 0;
    if (cst_read_registers(CST_REG_CHIP_ID, &chip_id, 1)) {
        Serial.printf("[HAL Touch CST816] Chip ID: 0x%02X", chip_id);
        if (chip_id == CST816S_CHIP_ID) Serial.println(" (CST816S)");
        else if (chip_id == CST816T_CHIP_ID) Serial.println(" (CST816T)");
        else if (chip_id == CST816D_CHIP_ID) Serial.println(" (CST816D)");
        else if (chip_id == CST820_CHIP_ID) Serial.println(" (CST820)");
        else Serial.println(" (UNKNOWN)");
    } else {
        Serial.println("[HAL Touch CST816] WARNING: Could not read chip ID");
    }

    // Read firmware version
    uint8_t fw_ver = 0;
    if (cst_read_registers(CST_REG_FW_VERSION, &fw_ver, 1)) {
        Serial.printf("[HAL Touch CST816] FW Version: 0x%02X\n", fw_ver);
    }

    // CRITICAL: Disable auto-sleep IMMEDIATELY after wake.
    // The CST816T vendor code requires reset() before this write, but since
    // we don't have RST, the INT pin toggle above serves as the wake event.
    // Must happen within ~5s of wake before controller re-enters sleep.
    Serial.println("[HAL Touch CST816] Disabling auto-sleep...");
    if (cst_write_register(CST_REG_DIS_AUTOSLEEP, 0x01)) {
        Serial.println("[HAL Touch CST816] Auto-sleep disabled (reg 0xFE = 0x01)");
    } else {
        Serial.println("[HAL Touch CST816] WARNING: Failed to disable auto-sleep");
    }

    // Configure interrupt mode to report touch coordinates (not just gestures)
    // Register 0xFA: 0x60 = touch-detect + state-change interrupts
    // Without this, controller may only report gesture events (home button)
    if (cst_write_register(0xFA, 0x60)) {
        Serial.println("[HAL Touch CST816] Interrupt mode set to touch+change (0x60)");
    } else {
        Serial.println("[HAL Touch CST816] WARNING: Failed to set interrupt mode");
    }

    delay(50);  // Let configuration settle

    // Read a diagnostic frame to check initial state
    uint8_t diag[13];
    if (cst_read_registers(CST_REG_STATUS, diag, 13)) {
        Serial.printf("[HAL Touch CST816] Init RAW: ");
        for (int i = 0; i < 13; i++) {
            Serial.printf("%02X ", diag[i]);
        }
        Serial.println();
    }

    // Read back auto-sleep register to verify write took effect
    uint8_t autosleep_val = 0;
    if (cst_read_registers(CST_REG_DIS_AUTOSLEEP, &autosleep_val, 1)) {
        Serial.printf("[HAL Touch CST816] Auto-sleep reg readback: 0x%02X (%s)\n",
                      autosleep_val, autosleep_val == 0x01 ? "DISABLED" : "ACTIVE!");
    }

    // Read back interrupt mode register
    uint8_t irq_mode = 0;
    if (cst_read_registers(0xFA, &irq_mode, 1)) {
        Serial.printf("[HAL Touch CST816] IRQ mode reg readback: 0x%02X\n", irq_mode);
    }

    g_display_width = static_cast<int16_t>(hal_display_get_width_pixels());
    g_display_height = static_cast<int16_t>(hal_display_get_height_pixels());
    Serial.printf("[HAL Touch CST816] Display: %dx%d\n", g_display_width, g_display_height);

    g_touch_initialized = true;
    Serial.println("[HAL Touch CST816] Initialized successfully (direct I2C mode)");
    return true;
}

bool hal_touch_read(hal_touch_point_t* point) {
    if (!g_touch_initialized || !point) {
        return false;
    }

    // Read touch registers 0x00-0x06 (7 bytes: status + gesture + count + X/Y)
    uint8_t buf[7];
    if (!cst_read_registers(CST_REG_STATUS, buf, 7)) {
        point->is_pressed = false;
        point->is_home_button = false;
        point->x = 0;
        point->y = 0;
        return true;
    }

    uint8_t num_points = buf[2] & 0x0F;

    // No touch or invalid (CST816 only supports 1 point)
    if (num_points == 0 || num_points > 1) {
        point->is_pressed = false;
        point->is_home_button = false;
        point->x = 0;
        point->y = 0;
        return true;
    }

    // Some CST816T return all 0xFF after auto-sleep disable
    if (buf[2] == 0xFF) {
        point->is_pressed = false;
        point->is_home_button = false;
        point->x = 0;
        point->y = 0;
        return true;
    }

    // Extract coordinates (FocalTech-style register layout)
    int16_t raw_x = ((buf[3] & 0x0F) << 8) | buf[4];
    int16_t raw_y = ((buf[5] & 0x0F) << 8) | buf[6];

    // Diagnostic: track if we ever see real coordinates
    static bool seen_real_touch = false;
    static uint32_t touch_count = 0;
    touch_count++;

    // Log raw register bytes for first few touches and periodically
    if (touch_count <= 5 || touch_count % 120 == 0) {
        Serial.printf("[HAL Touch CST816] #%u RAW bytes: %02X %02X %02X %02X %02X %02X %02X → x=%d y=%d\n",
                      touch_count, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6],
                      raw_x, raw_y);
    }

    if (!(raw_x == HOME_BTN_X && raw_y == HOME_BTN_Y) &&
        !(raw_x == 120 && raw_y == 600)) {
        if (!seen_real_touch) {
            seen_real_touch = true;
            Serial.printf("[HAL Touch CST816] FIRST REAL TOUCH: x=%d, y=%d\n", raw_x, raw_y);
        }
    }

    // Virtual home button detection
    // CST816T on T-Display 1.91" reports (600, 120) or swapped (120, 600)
    // Per touch_gesture_engine.md: map to BOTTOM EDGE_DRAG event
    if ((raw_x == HOME_BTN_X && raw_y == HOME_BTN_Y) ||
        (raw_x == HOME_BTN_Y && raw_y == HOME_BTN_X)) {
        point->is_pressed = false;
        point->is_home_button = true;
        point->x = 0;
        point->y = 0;
        return true;
    }

    // Validate raw coordinates (reject garbage data)
    // Valid ranges from HIL: X: 0-540, Y: 0-250
    if (raw_x < 0 || raw_x > 600 || raw_y < 0 || raw_y > 600) {
        point->is_pressed = false;
        point->is_home_button = false;
        point->x = 0;
        point->y = 0;
        return true;
    }

    // Debug: log on coordinate change
    static int16_t last_x = -1, last_y = -1;
    if (raw_x != last_x || raw_y != last_y) {
        Serial.printf("[HAL Touch CST816] RAW: x=%d, y=%d\n", raw_x, raw_y);
        last_x = raw_x;
        last_y = raw_y;
    }

    // Touch controller on T-Display S3 AMOLED Plus reports in display
    // coordinates directly — no rotation or scaling needed.
    // See IMPLEMENTATION_LOG.md "Touch Coordinate System" entries.
    int16_t transformed_x = raw_x;
    int16_t transformed_y = raw_y;

    // Clamp to display bounds
    if (transformed_x < 0) transformed_x = 0;
    if (transformed_x >= g_display_width) transformed_x = g_display_width - 1;
    if (transformed_y < 0) transformed_y = 0;
    if (transformed_y >= g_display_height) transformed_y = g_display_height - 1;

    point->x = transformed_x;
    point->y = transformed_y;
    point->is_pressed = true;
    point->is_home_button = false;

    return true;
}

void hal_touch_configure_gesture_engine(TouchGestureEngine* engine) {
    if (!engine) return;

    #if defined(APP_DISPLAY_ROTATION)
        // T-Display S3 AMOLED Plus (1.91") — 536x240 landscape
        // Touchable area from HIL: X: 2-536, Y: 46-239
        //
        // Edge zones must be TIGHT so center swipes work.
        // Previous zones (80, 215, 80, 180) left only 10% of screen as "center".
        // New zones: ~15% from each physical edge, leaving ~70% center area.
        engine->setEdgeZones(
            40,   // left_threshold: x < 40 (7.5% of 536)
            430,  // right_threshold: x > 430 (80% — last 20%)
            36,   // top_threshold: y < 36 (15% of 240)
            204   // bottom_threshold: y > 204 (85% — last 15%)
        );
    #else
        // ESP32-S3 AMOLED (1.8") — default percentage-based thresholds
    #endif
}

void hal_touch_sleep(void) {
    if (!g_touch_initialized) return;
    // Do NOT re-enable auto-sleep or send deep sleep (0xE5=0x03).
    // The T-Display 1.91" has no RST pin (RST=-1), so we have no reliable
    // mechanism to wake the CST816 from either auto-sleep or deep sleep.
    // The INT pin toggle used in hal_touch_init() is a hack that only works
    // on cold boot when the chip is already active (not yet in auto-sleep).
    // Keeping auto-sleep disabled means the CST816 stays active during light
    // sleep (~15mA), but it responds immediately to I2C probes on wake.
    g_touch_initialized = false;
    Serial.println("[HAL Touch CST816] Marked for re-init on wake (auto-sleep stays disabled)");
}

void hal_touch_wake(void) {
    // Full re-initialization: Wire.begin() (required after light sleep),
    // INT pin toggle to wake CST816 from deep sleep, auto-sleep disable,
    // and interrupt mode configuration.
    hal_touch_init();
    Serial.println("[HAL Touch CST816] Woke via full re-init");
}

#endif // !UNIT_TEST
