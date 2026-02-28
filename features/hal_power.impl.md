# HAL Power -- Implementation Notes

### Source Mapping

| File | Role |
|---|---|
| `hal/power.h` | C interface contract (includes battery_status types) |
| `hal/power_esp32_s3.cpp` | AXP2101 PMU (Waveshare) |
| `hal/power_tdisplay_s3_plus.cpp` | BQ25896/SY6970 charger (LilyGo) |
| `hal/power_stub.cpp` | Host-native test stub |
| `src/data/battery_status.h` | BatteryStatus data model (concrete DataItem) |
| `test/test_power_manager/` | Unit tests for power manager component |

### XPowersLib Bug Workarounds (SY6970)

- `getBattVoltage()` returns 0 when `getChargeCurrent()==0` (on battery, not charging). Bypassed by reading register 0x0E directly: `VBAT = (reg & 0x7F) * 20 + 2304 mV`.
- `chargeStatus()` always returns NO_CHARGE because it gates on `getChargeCurrent()>0`. Bypassed by reading register 0x0B bits [4:3] directly.

### Phantom Battery Detection (SY6970)

VBUS leakage causes the chip to report a battery when none is connected. Detection: pre-charge mode (status=1 or 2) with charge current < 50mA indicates phantom. Requires 3 consecutive confirming reads (sticky debounce) because I2C contention with the touch controller can cause `readChargeStatusRaw()` to fail, bypassing the check.

### SY6970 Shutdown Constraints

`disableBATFET()` cuts the battery power path permanently -- the SY6970 has no PEK (Power Enable Key), so there is no hardware mechanism to re-enable from a button press. The device is bricked until USB is connected. Deep sleep with GPIO 0 ext0 wakeup is used instead.

### ext0 Level-Trigger Caveat

ext0 wakeup is level-triggered. If GPIO 0 is still LOW (held) when deep sleep starts, the wakeup condition is immediately satisfied and the ESP32 reboots at once. Must wait for button release before entering deep sleep.

### I2C Bus Sharing Strategy

Both PMU implementations use callback-based I2C init (not `Wire.begin()` directly) to avoid reinitializing the bus already started by the touch HAL. Retry loops: 3 attempts with 200us delay.

### USB Boot OCV Correction

If Vbus > 4000mV at init, subtract 400mV from Vbat before seeding the smoothed charge cache. This compensates for the charge IC's voltage boost when powered via USB.
