# Feature: HAL Display

> Label: "HAL: Display"
> Category: "HAL"
> Prerequisite: features/arch_hal_contract.md
> Prerequisite: features/arch_display_pipeline.md

[TODO]

## 1. Overview

The Display HAL provides a hardware-agnostic C interface for all display operations on LPad devices. It abstracts AMOLED controller differences (SH8601 via QSPI on Waveshare, RM67162 via SPI on LilyGo T-Display S3 Plus) behind a unified API covering lifecycle management, primitive drawing, off-screen canvas compositing, bulk pixel transfer (DMA blit), shadow framebuffer inspection, and sleep/wake control. A test stub enables host-native unit testing without display hardware.

---

## 2. Requirements

### 2.1 Lifecycle

- `hal_display_init()` MUST return `true` on successful hardware initialization and `false` if the display controller is not found.
- `hal_display_init()` MUST be idempotent: calling it after successful init returns `true` immediately without reinitializing.
- On init, a PSRAM shadow framebuffer MUST be allocated and zeroed to mirror all subsequent pixel writes.

### 2.2 Drawing Primitives

- `hal_display_clear(color)` MUST fill the entire display with the given RGB565 color and update the shadow framebuffer.
- `hal_display_draw_pixel(x, y, color)` MUST write a single pixel and update the shadow framebuffer. Out-of-bounds coordinates MUST be silently dropped.
- `hal_display_flush()` is a no-op for direct-draw AMOLED controllers.

### 2.3 Geometry and Rotation

- `hal_display_get_width_pixels()` and `hal_display_get_height_pixels()` MUST return the current display dimensions, swapping axes when rotation is 90 or 270 degrees.
- `hal_display_set_rotation(degrees)` MUST accept 0, 90, 180, 270. Invalid values silently default to 0.
- `hal_display_get_corner_buffer_x()` and `hal_display_get_corner_buffer_y()` MUST return hardware-specific pixel offsets for corner logo positioning.

### 2.4 Canvas API

- `hal_display_canvas_create(w, h)` MUST allocate an off-screen PSRAM canvas and return a handle, or `nullptr` on OOM or if display is not initialized.
- `hal_display_canvas_select(canvas)` MUST redirect subsequent draw calls to the canvas. Passing `nullptr` redirects back to the main display.
- `hal_display_canvas_draw(canvas, x, y)` MUST blit the canvas to the main display at the given position and update the shadow framebuffer for the affected region.
- `hal_display_canvas_delete(canvas)` MUST free the canvas and auto-deselect if it was the currently selected canvas.
- `hal_display_canvas_fill(canvas, color)` MUST fill the canvas with the given RGB565 color.

### 2.5 Bulk Pixel Transfer

- `hal_display_fast_blit(x, y, w, h, data)` MUST use hardware bulk transfer (DMA), not pixel-by-pixel loops, and MUST update the shadow framebuffer.
- `hal_display_fast_blit_transparent(x, y, w, h, data, transparent_color)` MUST skip pixels matching the transparent color and MUST update the shadow framebuffer for non-transparent pixels.

### 2.6 Shadow Framebuffer

- `hal_display_read_pixel(x, y)` MUST read from the shadow framebuffer, not from display hardware. Out-of-bounds coordinates or missing PSRAM MUST return `0x0000`.
- `hal_display_dump_screen()` MUST transmit the full shadow framebuffer over serial using the protocol: `START:<width>,<height>\n` followed by raw RGB565 bytes (row-major, 2 bytes/pixel) followed by `\nEND\n`.

### 2.7 Sleep/Wake

- `hal_display_sleep()` MUST send MIPI Display Off (0x28) then Sleep In (0x10) commands to the controller.
- `hal_display_wake()` MUST send MIPI Sleep Out (0x11) then Display On (0x29) commands, wait at least 120ms for controller recovery, and restore display brightness.

### 2.8 Stub Requirements

- The stub implementation MUST compile on host-native without embedded SDK dependencies.
- `hal_display_init()` in the stub MUST return `false`.
- Stub reports 240x240 default resolution with rotation-aware axis swap.
- All draw/blit operations are no-ops. Canvas create returns `nullptr`. `read_pixel` returns `0x0000`.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Display initialization reports success

    Given the display hardware is present
    When hal_display_init is called
    Then it returns true
    And a PSRAM shadow framebuffer is allocated

#### Scenario: Double init is idempotent

    Given hal_display_init has already succeeded
    When hal_display_init is called again
    Then it returns true without reinitializing hardware

#### Scenario: Out-of-bounds pixel draws are silently dropped

    Given the display is initialized with dimensions WxH
    When hal_display_draw_pixel is called with coordinates outside 0..W-1, 0..H-1
    Then no crash occurs
    And the shadow framebuffer is unchanged for the out-of-bounds position

#### Scenario: Pixel writes mirror to shadow framebuffer

    Given the display is initialized
    When hal_display_draw_pixel(10, 20, 0xF800) is called
    Then hal_display_read_pixel(10, 20) returns 0xF800

#### Scenario: Clear fills entire shadow framebuffer

    Given the display is initialized
    When hal_display_clear(0x07E0) is called
    Then hal_display_read_pixel at any valid coordinate returns 0x07E0

#### Scenario: Canvas create returns nullptr when display not initialized

    Given hal_display_init has not been called or returned false
    When hal_display_canvas_create(100, 100) is called
    Then it returns nullptr

#### Scenario: Canvas draw updates shadow framebuffer

    Given a canvas is created and filled with color 0x001F
    When hal_display_canvas_draw is called at position (50, 50)
    Then hal_display_read_pixel within the canvas region returns 0x001F

#### Scenario: Rotation swaps width and height at 90 degrees

    Given the display is initialized with native dimensions 368x448
    When hal_display_set_rotation(90) is called
    Then hal_display_get_width_pixels returns 448
    And hal_display_get_height_pixels returns 368

#### Scenario: Invalid rotation defaults to zero

    Given the display is initialized
    When hal_display_set_rotation(45) is called
    Then the rotation is set to 0 degrees

#### Scenario: Screenshot dump protocol format

    Given the display is initialized with dimensions WxH
    And pixels have been drawn
    When hal_display_dump_screen is called
    Then serial output starts with "START:W,H\n"
    And contains W*H*2 bytes of raw RGB565 data
    And ends with "\nEND\n"

#### Scenario: Stub display init returns false

    Given the native_test environment is active
    When hal_display_init is called
    Then it returns false

#### Scenario: Stub read_pixel always returns zero

    Given the native_test stub is active
    When hal_display_read_pixel is called at any coordinate
    Then it returns 0x0000

### Manual Scenarios (Human Verification Required)

#### Scenario: Display renders correctly on Waveshare hardware

    Given the firmware is uploaded to Waveshare ESP32-S3 1.8" AMOLED
    When the device boots
    Then the boot logo appears centered on the 368x448 portrait display
    And colors match the theme palette

#### Scenario: Display renders correctly on T-Display S3 Plus hardware

    Given the firmware is uploaded to LilyGo T-Display S3 AMOLED Plus
    When the device boots
    Then the boot logo appears centered on the 448x368 landscape display
    And colors match the theme palette
