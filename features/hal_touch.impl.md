# HAL Touch -- Implementation Notes

### Source Mapping

| File | Role |
|---|---|
| `hal/touch.h` | C interface contract |
| `hal/touch_cst816.cpp` | CST816T direct I2C (LilyGo T-Display S3 Plus) |
| `hal/touch_ft3168.cpp` | FT3168 direct I2C (Waveshare) |
| `hal/touch_stub.cpp` | Host-native test stub |

### SensorLib Abandonment

Direct I2C was adopted because SensorLib's `TouchDrvCSTXXX` wrapper only returned the home button coordinate (600, 120) and never real touch points -- likely due to auto-sleep mode or initialization issues that could not be debugged through the library abstraction.

### CST816T I2C Wake Hack

The INT-pin wake sequence uses `Wire.end()` between I2C probe attempts because `Wire.begin()`'s internal `_started` flag survives across calls. Without `end()`, the I2C peripheral is not reinitialized after a failed probe, leaving stale driver state.

### CST816T Auto-Sleep

CRITICAL: Auto-sleep must be disabled within ~5 seconds of wake. The T-Display 1.91" has no RST pin (RST=-1), so there is no reliable mechanism to wake the CST816 from auto-sleep or deep sleep once entered.

### Edge Zone Calibration

Previous zones (80, 215, 80, 180) left only ~10% of screen as "center" area. Revised to ~15% from each physical edge, leaving ~70% center area for swipe detection.

### Register Layouts

- CST816T: 7 bytes from register 0x00 (gesture, touch count, xH/xL, yH/yL, chip ID)
- FT3168: 5 bytes from register 0x02 (touch status, xH/xL, yH/yL)
