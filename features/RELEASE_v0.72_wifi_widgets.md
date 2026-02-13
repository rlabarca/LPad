# Release 0.72: WiFi Selection & V1 UI Widgets

> Label: "Release v0.72: WiFi & Widgets"
> Category: "RELEASES"
> Prerequisite: features/ui_widget_framework.md
> Prerequisite: features/ui_standard_widgets.md
> Prerequisite: features/ui_wifi_list_widget.md
> Prerequisite: features/sys_serial_screenshot.md
> Prerequisite: features/sys_animation_ticker.md
> Prerequisite: features/sys_config_system.md
> Prerequisite: features/core_ui_render_manager.md
> Prerequisite: features/ui_system_menu.md
> Prerequisite: features/sys_mini_logo.md
> Prerequisite: features/app_stock_ticker.md
> Prerequisite: features/display_target_rotation.md

## 1. Objective
Enable manual WiFi selection through a new widget-based System Menu. This release introduces the core UI Widget System, providing a scalable way to build complex, interactive interfaces using relative coordinates.

## 2. Integration Requirements

### 2.1 The Widget Pipeline
- **WidgetLayoutEngine:** Must manage the registration and rendering of the `GridWidgetLayout`.
- **System Menu:** Must be refactored to use the Widget System instead of direct GFX calls for its central content.

### 2.2 Multi-WiFi HAL
- The system must support multiple SSIDs configured in `config.json`.
- The `hal_network` implementation must allow switching networks at runtime without a reboot.

## 3. Verification Criteria

### 3.1 HIL Test: Widget Layout & Rendering
- **Step 1:** Open System Menu.
- **Step 2:** Verify "WiFi Networks" heading is centered in the top cell of the grid.
- **Step 3:** Verify the WiFi List occupies the remaining space.
- **Step 4:** Verify text wrapping in a `TextWidget` by providing a long SSID (simulated or real).

### 3.2 HIL Test: Interactive WiFi Selection
- **Step 1:** Open System Menu.
- **Step 2:** Tap a non-connected WiFi network in the list.
- **Step 3:** Verify the item background changes to the "Connecting" color.
- **Step 4:** If connection fails, verify text turns red.
- **Step 5:** If connection succeeds, verify text turns the "Active" highlight color and the SSID in the top-right corner updates immediately.

### 3.3 HIL Test: List Scrolling
- **Step 1:** Configure 10+ WiFi networks in `config.json`.
- **Step 2:** Open System Menu.
- **Step 3:** Drag the list up/down.
- **Step 4:** Verify smooth scrolling and the presence of the 2px scroll indicator.

### 3.4 Regression: Serial Capture (0.71 Baseline)
- **Step 1:** Run `scripts/screenshot.sh`.
- **Step 2:** Verify that the capture shows the new Widget-based menu correctly.

## Implementation Notes
- **[2026-02-13]** The `WidgetLayout` calculations should be triggered on every `onUnpause()` of the System Menu to ensure orientation changes (if any) are handled.
- **[2026-02-13]** `WiFiListWidget` should poll `hal_network_get_status()` during its `update()` cycle while visible to handle the asynchronous nature of WiFi connection.
