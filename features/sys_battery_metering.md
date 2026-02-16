# Feature: Battery Status & Metering

> Label: "Battery Status"
> Category: "System Services"
> Prerequisite: features/arch_power_management.md
> Prerequisite: features/ui_system_menu.md
> Prerequisite: features/hal_display_corner_buffers.md

## 1. Introduction
This feature implements the background monitoring of battery health and the visual representation of that status in the System Menu.

## 2. Requirements

### 2.1 Power Manager Service
- Create `PowerManager` (a `SystemComponent`) registered with `UIRenderManager` at Z=0.
- `PowerManager` MUST poll `hal_power` every 2000ms.
- Results MUST be stored in a `BatteryStatus` data object (singleton or accessible via `PowerManager`).

### 2.2 BatteryStatus Data Model
- Inherits from `DataItem`.
- Fields:
    - `hal_power_status_t status`
    - `int8_t chargeLevel` (0-100)
    - `uint16_t voltageMv`

### 2.3 System Menu Integration
- The `SystemMenu` MUST display battery information in the **top-left** corner.
- **Format:**
    - If status is `HAL_POWER_STATUS_NO_BATTERY`: Display `[NO BATTERY]`
    - Otherwise: Display percentage (e.g., `85%`)
- **Styling:**
    - **Font:** Same as WiFi SSID (`fonts.normal`).
    - **Color:** 
        - Green if `CHARGING` or `CHARGED`.
        - Red if `DISCHARGING` and level < 15%.
        - Otherwise same as WiFi SSID color (`colors.text_status`).
- **Padding:**
    - The battery status and WiFi SSID MUST use `hal_display_get_corner_buffer_x()` and `hal_display_get_corner_buffer_y()` for their offsets to ensure they are visible on rounded displays.
    - Previous hard-coded `CORNER_PADDING_PX` (15px) is deprecated in favor of these HAL-provided values.

## 3. Scenarios

### Scenario: Monitoring Loop
- Given the system is running
- When 2000ms have elapsed since the last power poll
- Then `PowerManager` calls `hal_power_get_status()` and `hal_power_get_charge_level()`
- And updates the global `BatteryStatus` data object.

### Scenario: Visualizing Charging State
- Given the system is charging
- When the user opens the System Menu
- Then the top-left corner displays the current percentage in Green.

### Scenario: Visualizing Disconnected Battery
- Given no battery is connected
- When the user opens the System Menu
- Then the top-left corner displays `[NO BATTERY]` in the default status color.

## 4. Implementation Notes

### [2026-02-15] HAL Corner Buffers
The hard-coded 15px padding has been replaced by the `HAL Corner Buffer` API. This allows the UI to automatically adapt to different hardware (e.g., 20px for ESP32-S3 AMOLED vs 2px for T-Display).

### [2026-02-14] Battery Text Uses SSID Font
The battery status text reuses `m_ssidFont` (fonts.normal, 12pt) and shares the same Y-position (`SSID_Y_PERCENT`) as the WiFi SSID for visual alignment. The SSID text also uses the HAL Corner Buffer X-offset to symmetrically avoid corner clipping on both sides.

### [2026-02-14] CHARGED Shows Green (Same as CHARGING)
On boot while plugged in, the AXP2101 often reports `CHARGED` (charge done) rather than `CHARGING` if the battery is near-full. Both states now show green since both indicate external power is connected. Without this, the battery text appeared in default Chamoisee on boot, only turning green after unplug/replug triggered a `CHARGING` transition.

### [2026-02-14] Color Constants
`THEME_TEXT_CHARGING` (0x07E0, pure green) was added to `theme_colors.h`. Low-battery red reuses `THEME_TEXT_ERROR` (0xF800). Default color is `m_ssidColor` (Chamoisee / `text_status`).

[Complete]
