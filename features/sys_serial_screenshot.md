# Feature: Serial Screenshot Utility

> Label: "Serial Screenshot"
> Category: "System Architecture"
> Prerequisite: features/hal_spec_display.md
> Prerequisite: features/arch_infrastructure.md

## 1. Objective
Enable capturing the current contents of the display and transferring them to a host computer via Serial. This provides a way to document visual states, debug rendering issues, and verify HIL tests without physical cameras.

## 2. Requirement: Device-Side (Firmware)

### 2.1 Serial Command Listener
- The system MUST listen on the primary Serial port for the ASCII character 'S' (Screenshot Trigger).
- Upon receiving 'S', the `UIRenderManager` MUST temporarily pause all updates to ensure frame consistency during the dump.

### 2.2 HAL Extension: Pixel Dump
- A new internal or private HAL function `hal_display_dump_screen()` MUST be implemented.
- The dump MUST output the raw RGB565 pixel data for the entire screen (Width x Height).
- Data format: A continuous stream of raw bytes or a hex-encoded stream, preceded by a header indicating resolution (e.g., `START:W,H`).
- The dump MUST terminate with an `END` marker.

## 3. Requirement: Host-Side (Capture Utilities)

### 3.1 Script Locations
- The core logic MUST be in `scripts/capture_screenshot.py`.
- An executable shell script wrapper MUST be provided at `scripts/screenshot.sh`.

### 3.2 Script Behavior
- `scripts/screenshot.sh` MUST be the primary entry point for the user.
- It MUST handle permission checks (ensuring it is executable).
- It MUST activate the local `.venv` before calling the Python script to ensure `pyserial` and `Pillow` are available.
- It SHOULD provide clear feedback to the user on the progress of the capture.

### 3.3 Communication Protocol
- The script MUST utilize `pio device monitor` or a compatible `pyserial` wrapper to communicate with the device.
- It MUST send the 'S' character to the device.
- It MUST capture the serial output, parse the pixel data, and convert it into a standard `.png` file.
- The output file MUST be named `screenshot_[timestamp].png` and stored in the project root or a designated `captures/` folder.

## 4. Implementation Notes

### Pixel Conversion
RGB565 to RGB888 conversion logic:
```python
r = (pixel >> 11) & 0x1F
g = (pixel >> 5) & 0x3F
b = pixel & 0x1F
# Scale to 8-bit
r = (r * 255) // 31
g = (g * 255) // 63
b = (b * 255) // 31
```

### Performance
For a 450x600 display, raw RGB565 is ~540KB. At 115200 baud, this will take ~45 seconds. High-speed baud rates (e.g., 921600) are recommended for this feature.

### Shadow Framebuffer Strategy (v1 Implementation)
Neither the SH8601 (QSPI, no read-back) nor the RM67162 (SPI, no MISO) support reading pixels from the display controller. Instead, a **shadow framebuffer** is allocated in PSRAM during `hal_display_init()` and mirrored by all HAL draw functions:
- `hal_display_clear()`, `hal_display_draw_pixel()`, `hal_display_fast_blit()`, `hal_display_fast_blit_transparent()`, `hal_display_canvas_draw()`
- PSRAM cost: ~252KB (T-Display 240x536) or ~322KB (AMOLED 368x448)

**Known Limitation:** Draws made directly through the Arduino_GFX pointer (e.g., `_gfx->fillRect()` from RelativeDisplay OOP methods) bypass the HAL and are NOT captured in the shadow buffer. The primary content (graph canvases, HAL-routed draws) IS captured. For full-fidelity screenshots, all rendering should go through HAL functions.

### Frame Consistency
The screenshot trigger is processed at the top of `loop()`, between frames. Since the loop is single-threaded, the shadow buffer is in a consistent state (previous frame fully rendered). No explicit UIRenderManager pause is needed.

### Watchdog Safety
`hal_display_dump_screen()` yields to the RTOS after each row to prevent TG1WDT_SYS_RST during the lengthy serial transfer.
