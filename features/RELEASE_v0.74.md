# Release: v0.74 - Cumulative Feature Set

> Label: "Release 0.74"
> Category: "RELEASES"
> Prerequisite: features/ui_widget_framework.md
> Prerequisite: features/ui_standard_widgets.md
> Prerequisite: features/ui_wifi_list_widget.md
> Prerequisite: features/sys_battery_metering.md
> Prerequisite: features/hal_display_corner_buffers.md
> Prerequisite: features/app_stock_ticker.md
> Prerequisite: features/display_target_rotation.md
> Prerequisite: features/sys_animation_ticker.md
> Prerequisite: features/sys_config_system.md
> Prerequisite: features/sys_mini_logo.md
> Prerequisite: features/sys_serial_screenshot.md
> Prerequisite: features/sys_system_menu.md

## 1. Introduction
This release represents the current production-ready state of LPad, consolidating the V1 Widget System, asynchronous WiFi management, and hardware-aware telemetry (Battery & Corner Buffers).

## 2. Success Criteria

### 2.1 UI & Widgets (from v0.72)
- [x] Widget System manages the System Menu layout (1x5 Grid).
- [x] `WiFiListWidget` supports scrolling and asynchronous connection status.
- [x] `Manual WiFi selection overrides auto-connect sequence.

### 2.2 System Services & HAL (from v0.74)
- [x] `PowerManager` correctly polls and exposes `BatteryStatus` every 2s.
- [x] HAL provides `CORNER_BUFFER_X` and `CORNER_BUFFER_Y` constants per-device.
- [x] **LilyGo T-Display S3+**: X=2, Y=0.
- [x] **ESP32-S3 AMOLED**: X=20, Y=4.
- [x] System Menu overlays (Battery, SSID) use these HAL offsets instead of hard-coded padding.

## 3. Hardware (HIL) Verification

### Test 1: Widget & WiFi Interaction
- **Action:** Open System Menu, scroll the WiFi list, and select a network.
- **Verification:** Connection status updates visually (connecting/error/highlight) and SSID in top-right updates on success.

### Test 2: Battery Telemetry
- **Action:** Connect/Disconnect USB.
- **Verification:** Top-left indicator switches between Green (Charging) and default color (Discharging), and shows `[NO BATTERY]` if applicable.

### Test 3: Corner Buffer Alignment
- **Action:** Verify corner elements on **ESP32-S3 AMOLED**.
- **Verification:** Battery text (top-left) and SSID text (top-right) are inset by exactly 20px from the sides and 4px from the top, ensuring zero clipping from the rounded glass.

## 4. Implementation Notes

### [2026-02-15] HAL Corner Buffers
Hard-coded `CORNER_PADDING_PX` (15px) is removed. The UI now calls `hal_display_get_corner_buffer_x()` and `y()` to calculate the safe rendering zone.

### [2026-02-13] Widget Persistence
The `WidgetLayout` calculations are triggered on every `open()` of the System Menu to ensure orientation changes are handled.

[Complete]
