# HAL Display -- Implementation Notes

### Source Mapping

| File | Role |
|---|---|
| `hal/display.h` | C interface contract |
| `hal/display_esp32_s3_amoled.cpp` | Waveshare SH8601 QSPI implementation |
| `hal/display_tdisplay_s3_plus.cpp` | LilyGo RM67162 SPI implementation |
| `hal/display_stub.cpp` | Host-native test stub |
| `test/test_display_hal/` | Unit tests |
| `test/test_display_rotation/` | Rotation-specific tests |

### Hardware Differences

- **Waveshare (SH8601):** QSPI bus via `Arduino_ESP32QSPI`. 368x448 portrait. Power via XCA9554 GPIO expander (I2C 0x20). Corner buffers: x=20, y=4.
- **T-Display S3 Plus (RM67162):** SPI bus at 40 MHz via `Arduino_ESP32SPI`. 240x536 native (rotated to 448x368 landscape via `APP_DISPLAY_ROTATION=90`). Single PMIC_EN GPIO 38. Corner buffers: x=2, y=0. Uses TE signal sync before every blit.

### Vendor Init Quirks

- T-Display S3 Plus: `applyVendorInitSequence()` called twice; register 0x2B changed from 0x73 to 0x00 to fix Y-offset issue.
- Waveshare: verbose canvas logging was suppressed (flooded serial at 30fps).

### Shadow Framebuffer

- Allocated in PSRAM at init. Canvas operations do NOT mirror; only `canvas_draw` (blit to main display) mirrors.
- `dump_screen` yields per row to avoid watchdog reset.
