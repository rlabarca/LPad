# Feature: Touch Gesture Engine

> Label: "Touch Gesture Engine"
> Category: "System Architecture"
> Prerequisite: features/hal_spec_touch.md

## 1. Introduction
The Touch Gesture Engine interprets raw data from the HAL (`hal_touch_read`) and converts it into high-level semantic events (Tap, Swipe, Hold, Drag). It also provides a coordinate system that supports both absolute pixels and relative percentages, ensuring UI components can handle different aspect ratios.

## 2. Event Definitions

The engine must detect and report the following high-level semantic events:

### A. Tap
- **Condition:** A quick finger press and release.
- **Constraint:** The finger must remain within a small movement threshold of the original touch point to avoid being classified as a drag.
- **Outcome:** Fires on release.

### B. Hold
- **Condition:** A finger press maintained for a specific duration.
- **Constraint:** The finger must remain within a small movement threshold during the hold period.
- **Outcome:** Fires once when the duration threshold is reached.

### C. Hold And Drag
- **Condition:** A "Hold" event has occurred, and subsequently, the finger moves significantly from the original hold location.
- **Outcome:** Fires continuously as the touch position changes while the finger remains pressed.

### D. Edge Drag (Left, Right, Top, Bottom)
- **Condition:** A touch that starts near one of the screen's edges and moves towards the interior.
- **Start Constraint:** The touch must originate within a narrow zone along the screen's boundaries.
- **Direction:** The originating edge defines the event type (e.g., "Left Edge Drag" if starting from the left-most area).
- **Outcome:** Fires as the movement progresses or upon completion (implementation choice), distinct from standard interior interactions.
- **Hardware-Specific Note:** For devices with a virtual home button (e.g., T-Display S3 AMOLED Plus), interaction with this button MUST be mapped to a `BOTTOM EDGE DRAG` event (as if dragging from the bottom upwards), ensuring consistent navigation behavior regardless of physical button presence.

## 3. Data Interface

### 3.1 Event Structure
The engine must publish events containing:
- **Type:** `TOUCH_TAP`, `TOUCH_HOLD`, `TOUCH_HOLD_DRAG`, `TOUCH_EDGE_DRAG`
- **Direction:** `UP`, `DOWN`, `LEFT`, `RIGHT`, `NONE` (for Edge Drags)
- **Position (Absolute):** `x_px`, `y_px`
- **Position (Relative):** `x_percent` (0.0-1.0), `y_percent` (0.0-1.0)

## 4. Scenarios

### Scenario: Detecting a Tap
- Given the user presses at a location
- And releases quickly without significant movement
- Then a `TOUCH_TAP` event is generated

### Scenario: Detecting a Hold
- Given the user presses at a location
- And remains still for the required hold duration
- Then a `TOUCH_HOLD` event is generated

### Scenario: Detecting a Center Swipe
- Given the user presses in the center region of the screen
- And moves their finger in a clear direction (e.g., upward)
- Then a `TOUCH_SWIPE` event with direction `UP` is generated

### Scenario: Detecting an Edge Drag
- Given the user presses near the left edge of the screen
- And moves their finger towards the center
- Then a `TOUCH_EDGE_DRAG` event with direction `LEFT` (representing the origin) is generated

### Scenario: Relative Coordinates
- Given the screen has a known width and height
- And a touch event occurs at the exact center
- Then the reported `x_percent` and `y_percent` should be 0.50

## Implementation Notes

### [2026-02-10] Edge Zone Tuning History
The edge detection zone width was tuned iteratively across hardware:
- **Initial:** 5% of screen dimension — too narrow, missed edge swipes on the small 1.91" panel.
- **v2:** 10% — better detection but caused false positives in the graph interaction area.
- **Final:** 8% with a 20px minimum — balances sensitivity and false-positive rate across both the 1.8" (Waveshare) and 1.91" (T-Display) panels.

The bottom edge zone on the T-Display S3 Plus is special-cased: the virtual home button (reported by CST816T at coordinates ~(120,600)) is mapped to `TOUCH_EDGE_DRAG` direction `BOTTOM` regardless of the edge zone width, ensuring the system menu can always be invoked.

### [2026-02-11] Board-Specific Touch Controllers

| Board | Controller | I2C Addr | SDA | SCL | INT | Display |
|-------|-----------|----------|-----|-----|-----|---------|
| T-Display S3 AMOLED Plus (1.91") | CST816T | 0x15 | GPIO 3 | GPIO 2 | GPIO 21 | 536×240 landscape |
| Waveshare ESP32-S3 1.8" AMOLED | FT3168 | 0x38 | GPIO 15 | GPIO 14 | GPIO 21 | 368×448 portrait |

Same I2C pins ≠ same touch controller. Always check vendor demo code for the actual driver being used. Both controllers use direct I2C register reads — SensorLib was bypassed for reliability.

### [2026-02-11] FT3168 Direct I2C Implementation (Waveshare)
Simpler than CST816 — no wake-up sequence, no auto-sleep disable, no home button filtering needed. Register Map (FocalTech standard): `0x02` = num touches, `0x03-0x06` = X/Y coords. Chip ID 0x03 isn't in SensorLib's FT6X36 allowlist, so direct I2C reads (~20 lines) bypass driver compatibility risk entirely. Portrait mode (0° rotation), direct pass-through coordinates.

### [2026-02-11] FT3168 ≠ CST816 — Wrong Controller Assumption
**Problem:** The `esp32s3` environment was assumed to use CST816 because pin assignments (SDA=15, SCL=14, INT=21) are identical. Touch init failed silently.
**Discovery:** Vendor examples use `Arduino_FT3x68` driver. I2C address is 0x38 (FocalTech), not 0x15 (CST816).
**Impact:** Required `-<../hal/touch_ft3168.cpp>` or `-<../hal/touch_cst816.cpp>` exclusions in ALL 16 build environments to prevent duplicate symbol errors.

### [2026-02-11] Rate Limiter Anti-Pattern (CRITICAL)
**Never** use rate limiters that return `is_pressed = false` on skipped frames. The gesture engine transitions to `STATE_IDLE` on ANY frame where `is_pressed == false`, destroying sustained gesture detection (HOLD, SWIPE, DRAG). The state machine sees: PRESS → IDLE → PRESS → IDLE every N frames. If rate limiting is needed: return the LAST KNOWN touch state on skipped frames.

### [2026-02-11] Rate Limiter Phantom Releases (Waveshare)
**Problem:** Coordinates registered fine but gesture engine detected only TAP events — no HOLD, SWIPE, or DRAG.
**Root Cause:** Rate limiter returned `is_pressed = false` on 2 of every 3 frames.
**Fix:** Removed rate limiter entirely from `touch_ft3168.cpp`. FT3168 handles full-speed I2C polling without issues.

### [2026-02-11] Edge Zone Tuning — Detailed Values
**T-Display S3 AMOLED Plus (536×240 landscape):**
Left: x < 40 (7.5%), Right: x > 430 (80%), Top: y < 36 (15%), Bottom: y > 204 (85%). Leaves ~51% center for SWIPE.

**Waveshare ESP32-S3 1.8" (368×448 portrait):**
Default percentage-based thresholds (30% from each edge). No custom configuration needed.

**Tuning lessons:** Edge zones > 60% of screen leave almost no center for SWIPE. Axis-aware swipe thresholds (`getSwipeDistanceThreshold(dx, dy)`) prevent aspect ratio distortion on non-square screens.

### [2026-02-11] Gesture Direction Semantics
Edge drag direction reports the EDGE where the touch started, not the movement direction:
- `TOUCH_DIR_UP` = started from TOP edge (user swiped DOWN from top)
- `TOUCH_DIR_DOWN` = started from BOTTOM edge (user swiped UP from bottom)

### [2026-02-11] Debugging Protocol for Touch Issues
1. **Check RAW register bytes:** Log 7-byte I2C read to verify real data (not stuck values)
2. **Verify init sequence:** For CST816: INT pin wake → I2C init → auto-sleep disable → interrupt mode set
3. **Tap all 4 corners:** Compare expected vs actual coordinates for rotation/scaling needs
4. **Log start positions:** Shows which zone (LEFT/RIGHT/TOP/BOTTOM/CENTER) triggered classification
5. **Test with FT3168 first:** If gestures work there, the engine is fine — issue is CST816 HAL-level

### [2026-02-11] Touch Overlay Dirty Flag Optimization
**Problem:** Overlay blit every frame even when unchanged wasted DMA bandwidth.
**Solution:** Added `m_needs_blit` dirty flag — blit only on new gesture (~1fps) or after graph overwrite. Eliminated ~29 unnecessary blits/sec at 30fps.
**Lesson:** Track when content actually changes; DMA blits are fast but not free.
