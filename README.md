# ESP32-S3 Touch AMOLED Display Project

Hardware Abstraction Layer (HAL) based project for ESP32-S3-Touch-AMOLED-1.8 display.

## Project Structure

```
├── features/              # Feature specifications (Gherkin-style)
│   ├── hal_contracts.md   # Display HAL API contracts
│   └── display_baseline.md # ESP32-S3 AMOLED implementation
├── hal/                   # Hardware Abstraction Layer
│   ├── display.h          # Display HAL API
│   ├── display_stub.cpp   # Stub implementation for testing
│   └── display_esp32_s3_amoled.cpp # ESP32-S3 AMOLED implementation
├── src/                   # Application code
│   └── main.cpp           # Test application
├── test/                  # Unity tests
│   └── test_display_hal.cpp
├── hw-examples/           # Vendor reference code
└── scripts/
    └── test_local.sh      # Run Unity tests
```

## Hardware

**Target:** ESP32-S3-Touch-AMOLED-1.8

- Display: 368x448 AMOLED (SH8601 controller)
- Communication: QSPI (Quad SPI)
- Power Management: XCA9554 GPIO Expander
- Touch: FT3168 (not yet implemented)

## Building and Testing

### Run Unit Tests (Native)
```bash
./scripts/test_local.sh
# or
pio test -e native_test
```

### Build for ESP32-S3
```bash
pio run -e esp32s3
```

### Upload to Hardware
```bash
pio run -e esp32s3 -t upload
```

### Monitor Serial Output
```bash
pio device monitor -e esp32s3
```

## Display HAL API

See `features/hal_contracts.md` for complete API specification.

### Functions

- `bool hal_display_init(void)` - Initialize display hardware
- `void hal_display_clear(uint16_t color)` - Fill screen with color (RGB565)
- `void hal_display_draw_pixel(int32_t x, int32_t y, uint16_t color)` - Draw single pixel
- `void hal_display_flush(void)` - Flush buffer to screen

### RGB565 Color Format

16-bit color format: RRRRRGGGGGGBBBBB

Common colors:
- Black: 0x0000
- White: 0xFFFF
- Red: 0xF800
- Green: 0x07E0
- Blue: 0x001F

## Development Workflow

This project follows a Feature-First TDD approach:

1. Features are defined in `features/*.md` using Gherkin-style scenarios
2. Tests are written first (Unity framework)
3. Implementation follows the test
4. Git commits mark features as complete

See `CLAUDE.md` for full development protocols.

## License

See vendor example licenses in `hw-examples/` directories.
