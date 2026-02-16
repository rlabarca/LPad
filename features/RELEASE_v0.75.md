# Release v0.75: Cumulative Feature Set

> Label: "Release 0.75"
> Category: "RELEASES"
> Prerequisite: features/sys_power_states.md
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
This release introduces comprehensive power management (suspend, resume, shutdown) and consolidates all prior features from v0.74.

## 2. Success Criteria

### 2.1 Power Management (New for v0.75)
- [ ] **Suspend/Resume:** A short press of the power button/equivalent correctly suspends and resumes the device.
- [ ] **Shutdown/Startup:** A long press correctly shuts down and starts up the device.
- [ ] **Board Parity:** The above actions work correctly on both AXP2101 and SY6970-based boards, abstracting the hardware differences.

### 2.2 UI & Widgets (Regression from v0.74)
- [x] Widget System manages the System Menu layout (1x5 Grid).
- [x] `WiFiListWidget` supports scrolling and asynchronous connection status.
- [x] `Manual WiFi selection overrides auto-connect sequence.

### 2.3 System Services & HAL (Regression from v0.74)
- [x] `PowerManager` correctly polls and exposes `BatteryStatus` every 2s.
- [x] HAL provides `CORNER_BUFFER_X` and `CORNER_BUFFER_Y` constants per-device.
- [x] System Menu overlays (Battery, SSID) use these HAL offsets instead of hard-coded padding.

## 3. Hardware (HIL) Verification

### Test 1: Power States (New)
- **Action:** Perform short and long presses on the power button for both boards.
- **Verification:** Confirm that suspend, resume, shutdown, and startup actions all work as defined in `features/sys_power_states.md`.

### Test 2: Widget & WiFi Interaction (Regression)
- **Action:** Open System Menu, scroll the WiFi list, and select a network.
- **Verification:** Connection status updates visually (connecting/error/highlight) and SSID in top-right updates on success.

### Test 3: Battery Telemetry (Regression)
- **Action:** Connect/Disconnect USB.
- **Verification:** Top-left indicator switches between Green (Charging) and default color (Discharging), and shows `[NO BATTERY]` if applicable.

### Test 4: Corner Buffer Alignment (Regression)
- **Action:** Verify corner elements on **ESP32-S3 AMOLED**.
- **Verification:** Battery text (top-left) and SSID text (top-right) are inset by exactly 20px from the sides and 4px from the top.

[Complete]
