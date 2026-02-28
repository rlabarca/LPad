# Feature: Screenshot Capture Tool

> Label: "Tool: Screenshot"
> Category: "Developer Tools"
> Prerequisite: features/policy_build_pipeline.md
> Prerequisite: features/arch_hal_contract.md
> Prerequisite: features/arch_display_pipeline.md

[TODO]

## 1. Overview

The screenshot capture tool provides a two-part pipeline for extracting display framebuffer contents from a running LPad device. The device-side component listens for an 'S' character on Serial, then transmits the shadow framebuffer as raw RGB565 data using a START/END wire protocol. The host-side component is a Python script (with a shell wrapper) that auto-detects the ESP32-S3 serial port, triggers the capture, receives the RGB565 stream, converts it to RGB888, and saves a timestamped PNG file.

---

## 2. Requirements

### 2.1 Device-Side Trigger

- The main loop MUST check `Serial.available()` each frame and read any available character.
- If the character is `'S'`, `hal_display_dump_screen()` MUST be called to transmit the shadow framebuffer.
- The serial trigger MUST NOT block the render pipeline beyond the dump duration.

### 2.2 Wire Protocol

- Output format: `START:<width>,<height>\n` followed by `width * height * 2` bytes of raw RGB565 data (row-major, little-endian) followed by `\nEND\n`.
- The protocol is defined by the Display HAL contract (`hal_display_dump_screen()`). The screenshot tool is a consumer of that protocol.

### 2.3 Host-Side Python Script

- File: `scripts/capture_screenshot.py`.
- Dependencies: `pyserial`, `Pillow`. Missing dependencies MUST produce a clear error message and exit with code 1.
- Port auto-detection MUST match ESP32-S3 USB CDC by Espressif VID `0x303A`, falling back to USB-UART bridge chip descriptors (CP210x, CH340, ESP32).
- Manual port override via `-p`/`--port` argument.
- Baud rate configurable via `-b`/`--baud` (default: 115200).
- Output directory configurable via `-o`/`--output` (default: `captures/`). Directory MUST be created if absent.
- 30-second timeout waiting for the START marker. Timeout MUST produce an error message and return `None`.
- Progress MUST be displayed as a percentage during pixel data reception.
- RGB565-to-RGB888 conversion: R = 5-bit scaled to 8-bit, G = 6-bit scaled to 8-bit, B = 5-bit scaled to 8-bit.
- Output filename: `screenshot_<YYYYMMDD_HHMMSS>.png`.
- Incomplete data (fewer bytes than expected) MUST produce a WARNING but still attempt to save the partial image.

### 2.4 Shell Wrapper

- File: `scripts/screenshot.sh`.
- MUST auto-activate the project `.venv` if present and no virtual environment is active.
- MUST verify `python3` is available, `pyserial` is importable, and `Pillow` is importable. Each missing dependency MUST produce a specific install instruction.
- MUST forward all arguments to the Python script.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Serial trigger invokes screen dump

    Given the main loop is running
    When the character 'S' is received on Serial
    Then hal_display_dump_screen is called

#### Scenario: Non-trigger characters are ignored

    Given the main loop is running
    When a character other than 'S' is received on Serial
    Then hal_display_dump_screen is not called

#### Scenario: Auto-detect finds ESP32-S3 by VID

    Given a serial port with VID 0x303A is connected
    When find_device_port is called
    Then it returns that port's device path

#### Scenario: Auto-detect falls back to chip descriptor match

    Given a serial port with description containing "CP210"
    And no port with VID 0x303A exists
    When find_device_port is called
    Then it returns that port's device path

#### Scenario: Auto-detect returns None when no device found

    Given no serial ports match VID or descriptor patterns
    When find_device_port is called
    Then it returns None

#### Scenario: START marker timeout produces error

    Given a serial connection that never sends START
    When capture_screenshot waits for 30 seconds
    Then it returns None
    And an error message is printed

#### Scenario: RGB565 to RGB888 conversion is correct

    Given a raw RGB565 pixel value 0xF800 (pure red)
    When the pixel is converted to RGB888
    Then the result is (255, 0, 0)

#### Scenario: Incomplete data produces warning and partial image

    Given the device sends fewer bytes than width * height * 2
    When the Python script finishes reading
    Then a WARNING is printed with byte counts
    And a partial PNG is saved

### Manual Scenarios (Human Verification Required)

#### Scenario: Screenshot captures current display content

    Given the device is running with visible UI content
    When scripts/screenshot.sh is executed
    Then a PNG file is saved in captures/
    And the image matches the current display output
