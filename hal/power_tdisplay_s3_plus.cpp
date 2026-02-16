/**
 * @file power_tdisplay_s3_plus.cpp
 * @brief Power HAL implementation for LilyGo T-Display S3 Plus AMOLED (1.91" SPI)
 *
 * Uses the BQ25896/SY6970 charger IC over I2C (shared bus with touch on SDA=3, SCL=2).
 * Wire.begin() is already called by the touch HAL before hal_power_init(),
 * so we use CALLBACK-BASED init to avoid reinitializing the bus.
 *
 * The BQ25896 and SY6970 are register-compatible charger ICs. The LilyGo 1.91" SPI
 * board may have either chip; we try both I2C addresses (0x6A, 0x6B).
 *
 * NOTE: The XPowersLib PowersSY6970::getBattVoltage() has a bug — it returns 0
 * when getChargeCurrent()==0 (i.e., on battery, not charging). We bypass this by
 * reading register 0x0E directly: VBAT = (reg & 0x7F) * 20 + 2304 mV.
 *
 * See features/hal_spec_power.md for contract specification.
 */

#include "power.h"
#include <Arduino.h>
#include <Wire.h>

#include "PowersSY6970.tpp"

static PowersSY6970 pmu;
static bool g_pmu_ready = false;
static uint8_t g_pmu_addr = 0;
static int8_t g_last_level = -1;
static uint8_t g_ramp_ticks = 0;
static bool g_was_charging = false;   // Tracks charge→discharge transitions
static uint8_t g_no_batt_count = 0;   // Debounce counter for phantom battery detection

// LiPo discharge curve thresholds (millivolts)
static const uint16_t LIPO_MIN_MV = 3270;
static const uint16_t LIPO_MAX_MV = 4200;
static const uint16_t NO_BATTERY_MV = 3000;

// Direct register addresses (bypasses library's getChargeCurrent() gate bugs)
static const uint8_t REG_STATUS  = 0x0B;  // Charge status [4:3] + VBUS status [7:5]
static const uint8_t REG_VBAT    = 0x0E;  // Battery voltage ADC
static const uint8_t REG_ICHGR   = 0x12;  // Charge current ADC
static const uint16_t VBAT_BASE_MV = 2304;
static const uint8_t VBAT_STEP_MV = 20;
static const uint8_t ICHGR_STEP_MA = 50;  // Charge current step per LSB
static const uint16_t MIN_REAL_CHARGE_MA = 50;  // Below this = no real battery

// ---- Callback I2C (avoids Wire.begin() reinit on shared bus) ----

static int pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
    for (int attempt = 0; attempt < 3; attempt++) {
        Wire.beginTransmission(devAddr);
        Wire.write(regAddr);
        if (Wire.endTransmission(false) != 0) {
            delayMicroseconds(200);
            continue;
        }
        uint8_t got = Wire.requestFrom(devAddr, len);
        if (got == len) {
            for (uint8_t i = 0; i < len; i++) {
                data[i] = Wire.read();
            }
            return 0;
        }
        delayMicroseconds(200);
    }
    return -1;
}

static int pmu_register_write(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
    for (int attempt = 0; attempt < 3; attempt++) {
        Wire.beginTransmission(devAddr);
        Wire.write(regAddr);
        for (uint8_t i = 0; i < len; i++) {
            Wire.write(data[i]);
        }
        if (Wire.endTransmission() == 0) return 0;
        delayMicroseconds(200);
    }
    return -1;
}

/**
 * @brief Read charge status directly from register 0x0B bits [4:3].
 *        Bypasses PowersSY6970::chargeStatus() which gates on getChargeCurrent()>0,
 *        causing it to always return NO_CHARGE even when actively charging.
 *        Returns: 0=not charging, 1=pre-charge, 2=fast charge, 3=charge done
 */
static uint8_t readChargeStatusRaw(void) {
    uint8_t val = 0;
    if (pmu_register_read(g_pmu_addr, REG_STATUS, &val, 1) != 0) {
        return 0;
    }
    return (val >> 3) & 0x03;
}

/**
 * @brief Read charge current from register 0x12 bits [6:0], step 50mA.
 *        Used to distinguish "real battery charging" from "charger output with no battery."
 *        Without a battery, the charger reports pre-charge but current is ~0mA.
 */
static uint16_t readChargeCurrentRaw(void) {
    uint8_t val = 0;
    if (pmu_register_read(g_pmu_addr, REG_ICHGR, &val, 1) != 0) {
        return 0;
    }
    return (uint16_t)(val & 0x7F) * ICHGR_STEP_MA;
}

/**
 * @brief Read VBAT directly from register 0x0E, bypassing PowersSY6970::getBattVoltage()
 *        which incorrectly returns 0 when charge current is 0 (i.e., on battery).
 */
static uint16_t readBattVoltageRaw(void) {
    uint8_t val = 0;
    if (pmu_register_read(g_pmu_addr, REG_VBAT, &val, 1) != 0) {
        return 0;
    }
    uint8_t code = val & 0x7F;
    if (code == 0) return 0;
    return (uint16_t)code * VBAT_STEP_MV + VBAT_BASE_MV;
}

bool hal_power_init(void) {
    // Wire is already started by touch HAL (SDA=3, SCL=2).
    // Probe both addresses before calling begin() (begin() sets __has_init
    // on first call, so we can only call it once with the correct address).
    g_pmu_addr = 0;
    Wire.beginTransmission(SY6970_SLAVE_ADDRESS);
    if (Wire.endTransmission() == 0) {
        g_pmu_addr = SY6970_SLAVE_ADDRESS;
    } else {
        Wire.beginTransmission(BQ25896_SLAVE_ADDRESS);
        if (Wire.endTransmission() == 0) {
            g_pmu_addr = BQ25896_SLAVE_ADDRESS;
        }
    }

    if (g_pmu_addr == 0) {
        Serial.println("[PowerHAL] BQ25896/SY6970 not found on I2C bus");
        return true;  // Non-fatal
    }

    g_pmu_ready = pmu.begin(g_pmu_addr, pmu_register_read, pmu_register_write);
    if (!g_pmu_ready) {
        g_pmu_addr = 0;
        Serial.println("[PowerHAL] BQ25896/SY6970 init failed (chip ID mismatch?)");
        return true;  // Non-fatal
    }

    Serial.printf("[PowerHAL] BQ25896/SY6970 found at 0x%02X\n", g_pmu_addr);

    // Enable continuous ADC measurement and disable OTG boost
    pmu.enableADCMeasure(SY6970_ADC_CONTINUOUS);
    pmu.disableOTG();

    // Seed the rate-limiter cache
    uint16_t init_mv = readBattVoltageRaw();
    uint16_t vbus_mv = pmu.getVbusVoltage();
    Serial.printf("[PowerHAL] Init: Vbat=%dmV Vbus=%dmV", init_mv, vbus_mv);

    // If USB is connected at boot, terminal voltage is elevated above OCV
    if (vbus_mv > 4000 && init_mv > 400) {
        init_mv -= 400;
        Serial.printf(" (USB power, OCV estimate=%dmV)", init_mv);
    }
    Serial.println();

    if (init_mv >= NO_BATTERY_MV) {
        int32_t init_level = ((int32_t)init_mv - LIPO_MIN_MV) * 100 / (LIPO_MAX_MV - LIPO_MIN_MV);
        if (init_level < 0) init_level = 0;
        if (init_level > 100) init_level = 100;
        g_last_level = (int8_t)init_level;
    }

    return true;
}

hal_power_status_t hal_power_get_status(void) {
    if (!g_pmu_ready) {
        return HAL_POWER_STATUS_UNKNOWN;
    }

    uint16_t mv = readBattVoltageRaw();

    // No battery: voltage below floor or ADC reads 0
    if (mv < NO_BATTERY_MV) {
        return HAL_POWER_STATUS_NO_BATTERY;
    }

    // Direct register read: 0=not charging, 1=pre-charge, 2=fast charge, 3=done
    uint8_t chg = readChargeStatusRaw();

    // Phantom battery detection: when USB is connected but no battery,
    // the charger pushes voltage onto VBAT pin and reports "pre-charge."
    // Real charging draws 50+ mA; no battery draws ~0 mA.
    // Debounce: require 3 consecutive low-current reads before declaring
    // NO_BATTERY — I2C contention with touch controller can cause single
    // reads to fail (returning 0), falsely triggering this path.
    if (chg == 1 || chg == 2) {
        uint16_t ichg = readChargeCurrentRaw();
        if (ichg < MIN_REAL_CHARGE_MA) {
            g_no_batt_count++;
            if (g_no_batt_count >= 3) {
                return HAL_POWER_STATUS_NO_BATTERY;
            }
            return HAL_POWER_STATUS_CHARGING;  // Not enough evidence yet
        }
        g_no_batt_count = 0;
        return HAL_POWER_STATUS_CHARGING;
    }
    if (chg == 3) {
        return HAL_POWER_STATUS_CHARGED;
    }

    return HAL_POWER_STATUS_DISCHARGING;
}

int8_t hal_power_get_charge_level(void) {
    if (!g_pmu_ready) {
        return -1;
    }

    // Delegate to get_status() for consistent no-battery detection
    // (covers both voltage floor AND phantom-battery current check)
    hal_power_status_t st = hal_power_get_status();
    if (st == HAL_POWER_STATUS_NO_BATTERY || st == HAL_POWER_STATUS_UNKNOWN) {
        g_was_charging = false;
        return -1;
    }

    bool is_charging = (st == HAL_POWER_STATUS_CHARGING || st == HAL_POWER_STATUS_CHARGED);

    // When charger disconnects, reset ramp counter but keep g_last_level.
    // Surface charge keeps voltage elevated for several seconds after unplug,
    // so accepting raw readings would cause the level to creep upward.
    if (g_was_charging && !is_charging) {
        g_ramp_ticks = 0;
    }
    g_was_charging = is_charging;

    uint16_t mv = readBattVoltageRaw();
    if (mv < NO_BATTERY_MV) {
        return -1;
    }

    int32_t level = ((int32_t)mv - LIPO_MIN_MV) * 100 / (LIPO_MAX_MV - LIPO_MIN_MV);
    if (level < 0) level = 0;
    if (level > 100) level = 100;

    // Rate-limit upward jumps based on charge state:
    // - Charging: slow ramp +1% every 5 polls (10s) to smooth voltage spikes
    // - Not charging: freeze (never increase) — battery can't gain charge,
    //   any upward reading is surface charge or measurement noise
    if (g_last_level >= 0 && level > g_last_level) {
        if (is_charging) {
            g_ramp_ticks++;
            if (g_ramp_ticks >= 5) {
                g_ramp_ticks = 0;
                level = g_last_level + 1;
            } else {
                level = g_last_level;
            }
        } else {
            level = g_last_level;  // Not charging: no upward movement
        }
    } else {
        g_ramp_ticks = 0;
    }

    g_last_level = (int8_t)level;
    return (int8_t)level;
}

uint16_t hal_power_get_voltage_mv(void) {
    if (!g_pmu_ready) {
        return 0;
    }
    return readBattVoltageRaw();
}

// ---- Power Button & State Management (sys_power_states.md) ----
// The SY6970 has no PEK feature, so we emulate button events using GPIO 0
// with a software debounce state machine.

#include "esp_sleep.h"

#define POWER_BUTTON_GPIO    0
#define SHORT_PRESS_MAX_MS   1000
#define LONG_PRESS_THRESHOLD_MS  4000

static enum {
    BTN_IDLE,
    BTN_PRESSED,
    BTN_WAIT_RELEASE,  // After long press fires, wait for release
} g_btn_state = BTN_IDLE;

static uint32_t g_btn_press_start = 0;

void hal_power_button_init(void) {
    pinMode(POWER_BUTTON_GPIO, INPUT_PULLUP);
    g_btn_state = BTN_IDLE;
    Serial.println("[PowerHAL] GPIO 0 button initialized (software debounce)");
}

hal_power_button_event_t hal_power_button_get_event(void) {
    bool pressed = (digitalRead(POWER_BUTTON_GPIO) == LOW);
    uint32_t now = millis();

    switch (g_btn_state) {
        case BTN_IDLE:
            if (pressed) {
                g_btn_press_start = now;
                g_btn_state = BTN_PRESSED;
            }
            break;

        case BTN_PRESSED:
            if (!pressed) {
                // Released — check duration
                uint32_t duration = now - g_btn_press_start;
                g_btn_state = BTN_IDLE;
                if (duration < SHORT_PRESS_MAX_MS) {
                    return HAL_POWER_EVENT_SHORT_PRESS;
                }
                // Duration between 1-4s: ignored (not short, not long enough)
            } else {
                // Still pressed — check for long press threshold
                uint32_t duration = now - g_btn_press_start;
                if (duration >= LONG_PRESS_THRESHOLD_MS) {
                    g_btn_state = BTN_WAIT_RELEASE;
                    return HAL_POWER_EVENT_LONG_PRESS;
                }
            }
            break;

        case BTN_WAIT_RELEASE:
            if (!pressed) {
                g_btn_state = BTN_IDLE;
            }
            break;
    }

    return HAL_POWER_EVENT_NONE;
}

void hal_power_suspend(void) {
    Serial.println("[PowerHAL] Entering light sleep (GPIO 0 wakeup)...");
    Serial.flush();
    delay(50);  // Allow serial to flush

    // Configure GPIO 0 as ext0 wakeup source (active LOW = button pressed)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)POWER_BUTTON_GPIO, 0);

    // Enter light sleep — blocks until wakeup
    esp_light_sleep_start();

    // Woken up — reset button state machine to avoid immediate re-trigger
    g_btn_state = BTN_WAIT_RELEASE;

    Serial.println("[PowerHAL] Wakeup from light sleep");
}

void hal_power_resume(void) {
    // Re-seed the rate-limiter cache after wake
    if (g_pmu_ready) {
        uint16_t mv = readBattVoltageRaw();
        if (mv >= NO_BATTERY_MV) {
            int32_t level = ((int32_t)mv - LIPO_MIN_MV) * 100 / (LIPO_MAX_MV - LIPO_MIN_MV);
            if (level < 0) level = 0;
            if (level > 100) level = 100;
            g_last_level = (int8_t)level;
        }
        g_ramp_ticks = 0;
        g_was_charging = false;
    }
}

void hal_power_shutdown(void) {
    if (!g_pmu_ready) return;
    Serial.println("[PowerHAL] SY6970 shutdown — disabling BATFET");
    Serial.flush();
    delay(100);
    pmu.shutdown();  // Calls disableBATFET() — cuts battery power path
    // Does not return if on battery. If USB connected, BATFET disable
    // is ignored by the SY6970, so we enter an infinite loop.
    while (true) { delay(1000); }
}
