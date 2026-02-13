# Release 0.71: Serial Screenshot Utility

> Label: "Release v0.71: Serial Screenshot"
> Category: "RELEASES"
> Prerequisite: features/sys_serial_screenshot.md
> Prerequisite: features/sys_animation_ticker.md
> Prerequisite: features/sys_config_system.md
> Prerequisite: features/core_ui_render_manager.md
> Prerequisite: features/sys_system_menu.md
> Prerequisite: features/sys_mini_logo.md
> Prerequisite: features/app_stock_ticker.md
> Prerequisite: features/display_target_rotation.md

## 1. Objective
Introduce a host-side screenshot utility that allows developers to capture the current visual state of the device over Serial. This release also serves as a regression baseline for the UI Render Manager architecture.

## 2. Integration Requirements

### 2.1 The Screenshot Pipeline
- Device MUST listen for 'S' on Serial.
- Device MUST dump a consistent shadow framebuffer from PSRAM.
- Host MUST provide an executable `scripts/screenshot.sh` that manages the `.venv` and Python capture logic.

### 2.2 System Architecture (Baseline)
The system is managed by the `UIRenderManager` with the following component registry:
- **Z=1:** `StockTicker` (Active App)
- **Z=10:** `MiniLogo` (System Overlay)
- **Z=20:** `SystemMenu` (System Overlay, Edge Drag triggered)

## 3. Verification Criteria

### 3.1 HIL Test: Serial Capture
- **Step 1:** Run `scripts/screenshot.sh`.
- **Step 2:** Verify that a PNG file is generated in the root directory.
- **Step 3:** Verify the image accurately reflects the current screen (Graph + MiniLogo).

### 3.2 HIL Test: Compositing (Full Rigor)
- **Step 1:** Boot device.
- **Step 2:** Verify Stock Graph appears as the background (Z=1).
- **Step 3:** Verify Mini Logo is rendered on top (Z=10).
- **Step 4:** Verify the Logo's background is transparent (graph is visible through logo "empty" space).

### 3.3 HIL Test: System Menu Interaction (Full Rigor)
- **Step 1:** Perform `EDGE_DRAG: TOP` (swipe down from top edge).
- **Step 2:** Verify System Menu slides down/appears (Z=20).
- **Step 3:** Verify Graph stops updating (Paused/Occluded).
- **Step 4:** Perform `EDGE_DRAG: BOTTOM` (swipe up from bottom edge).
- **Step 5:** Verify Menu disappears.
- **Step 6:** Verify Graph resumes updating.

## Implementation Notes
- Shadow framebuffer is allocated in PSRAM to bypass the lack of controller read-back support on SH8601/RM67162.
- Serial transfer is row-buffered and yields to the RTOS to prevent Watchdog resets.
