# Feature: HAL Display Corner Buffers

> Label: "HAL Corner Buffers"
> Category: "Hardware Layer"
> Prerequisite: features/hal_spec_display.md

## 1. Introduction
Modern displays often have aggressively rounded corners that can cut off UI elements placed near the screen edges. This feature provides a standardized way for the Hardware Abstraction Layer (HAL) to communicate the required "safe" offsets (buffers) for each corner.

## 2. Specification

### 2.1 Corner Buffer Constants
The HAL MUST provide two properties (or constants) to define the horizontal and vertical safety margins:
*   `HAL_DISPLAY_CORNER_BUFFER_X`: The minimum pixel offset from the left/right edge to avoid content clipping.
*   `HAL_DISPLAY_CORNER_BUFFER_Y`: The minimum pixel offset from the top/bottom edge to avoid content clipping.

### 2.2 Corner Application Logic
When rendering elements near corners, these buffers MUST be applied as follows:
*   **Top-Left:** `x = CORNER_BUFFER_X`, `y = CORNER_BUFFER_Y`
*   **Top-Right:** `x = Width - CORNER_BUFFER_X - ElementWidth`, `y = CORNER_BUFFER_Y`
*   **Bottom-Left:** `x = CORNER_BUFFER_X`, `y = Height - CORNER_BUFFER_Y - ElementHeight`
*   **Bottom-Right:** `x = Width - CORNER_BUFFER_X - ElementWidth`, `y = Height - CORNER_BUFFER_Y - ElementHeight`

### 2.3 Hardware Specific Values
The values are determined by the physical radius of the display housing:

| Device | Corner Buffer X | Corner Buffer Y |
| :--- | :--- | :--- |
| **LilyGo T-Display S3 Plus** | 2 px | 0 px |
| **Waveshare ESP32-S3 AMOLED 1.8** | 15 px | 4 px |
| **Stub/Default** | 0 px | 0 px |

## 3. Implementation Requirements
1.  Add `hal_display_get_corner_buffer_x()` and `hal_display_get_corner_buffer_y()` to `hal/display.h`.
2.  Implement these in `hal/display_tdisplay_s3_plus.cpp`, `hal/display_esp32_s3_amoled.cpp`, and `hal/display_stub.cpp`.
3.  Update UI components (specifically `SystemMenu`) to replace hard-coded `CORNER_PADDING_PX` with these HAL values.

## 4. Acceptance Criteria
*   [ ] WiFi SSID in System Menu (top-right) is not clipped on ESP32-S3 AMOLED.
*   [ ] Battery percentage (top-left) is not clipped on ESP32-S3 AMOLED.
*   [ ] T-Display maintains its minimal 2px horizontal padding.
*   [ ] Code compiles for all targets.
