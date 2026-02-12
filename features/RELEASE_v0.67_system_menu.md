# Release v0.67: System Menu

> Label: "Release v0.67 (System Menu)"
> Category: "Releases"
> Prerequisite: features/ui_system_menu.md, features/RELEASE_v0.60_initial_stock_tracker.md

## 1. Overview
This release introduces the "System Menu" as the primary navigation and status interface, replacing the persistent on-screen version overlays used in previous releases. It cleans up the main UI by moving metadata to a hidden layer, accessible via gesture.

## 2. Capability Changes

### 2.1 UI Cleanup (Removals)
- **Remove Touch Test Overlay:** The visual debug overlay from v0.65 (showing coordinates and last event) is removed. Touch interaction remains functional but silent.
- **Remove "DEMO vX" Overlay:** The persistent "DEMO vX" text in the top-left corner of the graph screen (introduced in v0.5 and refined in subsequent releases) is removed.

### 2.2 Functional Carry-over
- **Graph Functionality:** The stock tracker and time-series graph from **Release v0.60** are preserved in their entirety, including Yahoo Finance integration and layout engine.
- **Touch Gestures:** All gesture detection from **Release v0.65** is preserved to support the System Menu triggers.

### 2.3 New Features
- **System Menu Implementation:** As defined in `features/ui_system_menu.md`.
- **Ticker Watermark:** The graph is configured to display the active ticker symbol as a background watermark (top-center, subtle color, heading font) via the new `setWatermark` API.
- **Version Migration:** The version string "Version 0.67" is now located exclusively within the System Menu.

## 3. Hardware-In-Loop (HIL) Test

### 3.1 Setup
- Flash the `demo_v0.67` firmware.
- The device should boot directly to the Stock Tracker graph.

### 3.2 Test Scenarios

**Scenario A: Clean Graph View**
1. Observe the main graph screen.
2. Verify: No "DEMO v0.67" text is visible in the corner.
3. Verify: The ticker symbol (e.g., "^TNX") is visible in the top-center background.
4. Verify: The ticker is drawn in a subtle color and is "behind" the graph line if they intersect.
5. Verify: No touch crosshair or coordinate overlay appears when touching the screen.

**Scenario B: Opening System Menu**
1. Swipe down from the top edge.
2. Verify: The black System Menu animates down smoothly.
3. Verify: The text "Version 0.67" appears in the top-left (small/subtle).
4. Verify: The current WiFi SSID appears in the top-right.

**Scenario C: Dismissing System Menu**
1. Swipe up from the bottom of the screen while the menu is open.
2. Verify: The menu animates up and reveals the underlying graph.
3. Verify: The graph is up-to-date and correctly rendered.

## 4. Success Criteria
- The "DEMO" text is gone from the main screen.
- The System Menu is responsive and follows the 30 FPS animation requirement.
- No rendering artifacts (flashing) occur when the menu is opening or closing.
