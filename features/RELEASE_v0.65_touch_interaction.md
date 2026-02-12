# Release v0.65: Touch Interaction & Debugging

> Label: "Release v0.65 (Touch)"
> Category: "Releases"
> Prerequisite: features/ui_touch_test_overlay.md

## 1. Overview
This release introduces full touchscreen support to the LPad platform. It establishes the hardware abstraction for touch, implements the CST816 driver for supported boards, adds a sophisticated gesture engine, and provides a visual debug overlay to verify functionality.

## 2. Components

### 2.0 Versioning
- **Demo Title:** The persistent title overlay on the graph (or the demo coordinator) must be updated to display **"DEMO v0.65"**.

### 2.1 Hardware Layer
- **HAL Contract:** `hal_touch_init`, `hal_touch_read`.
- **Implementation:** Touch driver support for both boards: CST816 for `tdisplay_s3_plus` (1.91"), FT3168 for `esp32s3` (1.8"), using correct pin mappings and direct I2C register access.

### 2.2 Application Layer
- **Touch Gesture Engine:** logic to parse raw inputs into:
    - TAP
    - HOLD
    - HOLD_AND_DRAG
    - EDGE_DRAG (Directional)
    - **Virtual Home Button:** Must be mapped to `BOTTOM EDGE DRAG` on supported devices (e.g., T-Display S3 AMOLED Plus).

### 2.3 UI Layer
- **Touch Test Overlay:** A transient, global overlay that displays the last detected event and coordinates.
    - Auto-hides after 3 seconds.
    - Renders on top of the active screen (Graph).
    - **Visual Indicator:** Displays a small "+" crosshair at the precise touch location during interaction, respecting HAL rotation and scaling.

## 3. Hardware-In-Loop (HIL) Verification Plan

### 3.1 Setup
- Flash the `demo_v0.65` firmware to the target device.
- Ensure the Stock Tracker graph is visible (default screen).

### 3.2 Test Cases

**Test A: Basic Tap**
1. Tap the center of the screen.
2. Verify overlay appears: `TAP: (x, y)`.
3. Wait 3 seconds.
4. Verify overlay disappears.

**Test B: Hold**
1. Press and hold finger in center.
2. Verify overlay appears: `HOLD: ...`.

**Test D: Edge Drag**
1. Drag from the left edge towards center.
2. Verify overlay: `EDGE_DRAG: LEFT`.

**Test E: Hold and Drag**
1. Press, hold (wait for Hold event), then drag finger.
2. Verify overlay updates continuously with coordinates: `HOLD_DRAG: ...`.
3. Verify a small "+" crosshair follows the finger exactly.

**Test F: Virtual Home Button (T-Display S3 Plus Only)**
1. Tap/Interact with the virtual home button area.
2. Verify overlay: `EDGE_DRAG: BOTTOM`.

## 4. Success Criteria
- All supported gestures are reliably detected.
- The overlay renders clearly over the graph.
- The visual crosshair accurately tracks the finger position.
- The application does not crash or stall during touch interaction.
