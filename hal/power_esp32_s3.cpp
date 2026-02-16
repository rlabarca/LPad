/**
 * @file power_esp32_s3.cpp
 * @brief Power HAL implementation for Waveshare ESP32-S3 Touch AMOLED 1.8"
 *
 * Uses the AXP2101 PMU chip over I2C (shared bus with touch on SDA=15, SCL=14).
 * Wire.begin() is already called by the touch HAL before hal_power_init(),
 * so we use CALLBACK-BASED init to avoid reinitializing the bus.
 *
 * See features/hal_spec_power.md for contract specification.
 */

#include "power.h"
#include <Arduino.h>
#include <Wire.h>

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

static XPowersPMU pmu;
static bool g_pmu_ready = false;
static int8_t g_last_level = -1;  // Rate-limiter for charging % (prevents voltage-spike jumps)
static uint8_t g_ramp_ticks = 0;  // Sub-poll counter for slow ramp (+1% per N polls)

// LiPo discharge curve thresholds (millivolts)
static const uint16_t LIPO_MIN_MV = 3270;
static const uint16_t LIPO_MAX_MV = 4200;
static const uint16_t NO_BATTERY_MV = 3000;  // Below this = no real battery (charger pre-charge ≈ 3.0V)

// ---- Callback I2C (avoids Wire.begin() reinit on shared bus) ----

static int pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len) {
    // Retry up to 3 times — shared bus with touch controller causes occasional NACK
    for (int attempt = 0; attempt < 3; attempt++) {
        Wire.beginTransmission(devAddr);
        Wire.write(regAddr);
        if (Wire.endTransmission(false) != 0) {  // repeated start
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

bool hal_power_init(void) {
    // Wire is already started by touch HAL (SDA=15, SCL=14).
    // Use callback-based init to avoid calling Wire.begin() again.
    g_pmu_ready = pmu.begin(AXP2101_SLAVE_ADDRESS, pmu_register_read, pmu_register_write);

    if (!g_pmu_ready) {
        Serial.println("[PowerHAL] AXP2101 not found on I2C bus");
        return true;  // Non-fatal: system runs fine without battery monitoring
    }

    // Enable internal ADC measurements
    pmu.enableBattDetection();
    pmu.enableBattVoltageMeasure();
    pmu.enableVbusVoltageMeasure();
    pmu.enableSystemVoltageMeasure();

    // Seed the rate-limiter cache. If USB is connected at boot, terminal
    // voltage is elevated above OCV even before isCharging() reports true.
    // Use VBUS voltage to detect USB power (more reliable than isCharging()
    // which lags during PMU init).
    uint16_t init_mv = pmu.getBattVoltage();
    uint16_t vbus_mv = pmu.getVbusVoltage();
    Serial.printf("[PowerHAL] AXP2101 init: Vbat=%dmV Vbus=%dmV", init_mv, vbus_mv);

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

    if (!pmu.isBatteryConnect()) {
        return HAL_POWER_STATUS_NO_BATTERY;
    }

    // isBatteryConnect() can report true with no battery due to VBUS
    // leakage on the VBAT pin. Voltage check catches this.
    if (pmu.getBattVoltage() < NO_BATTERY_MV) {
        return HAL_POWER_STATUS_NO_BATTERY;
    }

    if (pmu.isCharging()) {
        return HAL_POWER_STATUS_CHARGING;
    }

    // Check if fully charged (USB in but not actively charging)
    uint8_t charger = pmu.getChargerStatus();
    if (charger == XPOWERS_AXP2101_CHG_DONE_STATE) {
        return HAL_POWER_STATUS_CHARGED;
    }

    return HAL_POWER_STATUS_DISCHARGING;
}

int8_t hal_power_get_charge_level(void) {
    if (!g_pmu_ready || !pmu.isBatteryConnect()) {
        // Do NOT reset g_last_level here — transient I2C glitches on the
        // shared bus can briefly return false, which would clear the cache
        // and let the next valid read bypass the rate limiter.
        return -1;
    }

    uint16_t mv = pmu.getBattVoltage();
    if (mv == 0) return -1;

    int32_t level = ((int32_t)mv - LIPO_MIN_MV) * 100 / (LIPO_MAX_MV - LIPO_MIN_MV);
    if (level < 0) level = 0;
    if (level > 100) level = 100;

    // Rate-limit upward jumps based on charge state:
    // - Charging: slow ramp +1% every 5 polls (10s) to smooth voltage spikes
    // - Not charging: freeze (never increase) — battery can't gain charge,
    //   any upward reading is surface charge or measurement noise
    bool is_charging = pmu.isCharging();
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
    if (!g_pmu_ready || !pmu.isBatteryConnect()) {
        return 0;
    }
    return pmu.getBattVoltage();
}

// ---- Power Button & State Management (sys_power_states.md) ----

void hal_power_button_init(void) {
    if (!g_pmu_ready) return;

    // Disable all IRQs, then enable only power key events
    pmu.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ);

    // Configure long press to 4 seconds for hardware-managed shutdown
    pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);

    // Clear any pending IRQ from boot
    pmu.getIrqStatus();
    pmu.clearIrqStatus();

    Serial.println("[PowerHAL] AXP2101 PEK button initialized (4s shutdown)");
}

hal_power_button_event_t hal_power_button_get_event(void) {
    if (!g_pmu_ready) return HAL_POWER_EVENT_NONE;

    // Read and process IRQ status from AXP2101 via I2C
    pmu.getIrqStatus();

    hal_power_button_event_t event = HAL_POWER_EVENT_NONE;

    if (pmu.isPekeyShortPressIrq()) {
        event = HAL_POWER_EVENT_SHORT_PRESS;
    } else if (pmu.isPekeyLongPressIrq()) {
        event = HAL_POWER_EVENT_LONG_PRESS;
    }

    pmu.clearIrqStatus();
    return event;
}

void hal_power_suspend(void) {
    // The Waveshare board's PMU IRQ is routed through the I2C GPIO expander
    // (XCA9554 pin 5), not a direct ESP32 GPIO. This means we cannot use
    // esp_light_sleep_start() with ext0/ext1 wakeup. Instead, we enter a
    // low-power polling loop that checks the PMU for button events every
    // 100ms. Display, WiFi, and touch are already off at this point.
    Serial.println("[PowerHAL] Entering soft suspend (PMU polling)...");
    Serial.flush();

    while (true) {
        hal_power_button_event_t ev = hal_power_button_get_event();
        if (ev == HAL_POWER_EVENT_SHORT_PRESS) {
            break;  // Wake up
        }
        if (ev == HAL_POWER_EVENT_LONG_PRESS) {
            hal_power_shutdown();  // Does not return
        }
        delay(100);  // Low-power polling interval
    }

    Serial.println("[PowerHAL] Wakeup from suspend");
}

void hal_power_resume(void) {
    // Re-seed the rate-limiter cache after wake.
    // Battery voltage may have drifted during suspend.
    if (g_pmu_ready) {
        uint16_t mv = pmu.getBattVoltage();
        if (mv >= NO_BATTERY_MV) {
            int32_t level = ((int32_t)mv - LIPO_MIN_MV) * 100 / (LIPO_MAX_MV - LIPO_MIN_MV);
            if (level < 0) level = 0;
            if (level > 100) level = 100;
            g_last_level = (int8_t)level;
        }
        g_ramp_ticks = 0;
    }
}

void hal_power_shutdown(void) {
    if (!g_pmu_ready) return;
    Serial.println("[PowerHAL] AXP2101 shutdown — cutting all power rails");
    Serial.flush();
    delay(100);
    pmu.shutdown();
    // Does not return — PMU cuts power
    while (true) { delay(1000); }
}
