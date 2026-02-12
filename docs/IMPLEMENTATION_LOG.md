# Implementation Log & Lessons Learned

This document captures the "tribal knowledge" of the project: technical hurdles, why specific decisions were made, and what approaches were discarded.

## [2026-02-11] Touch System: Final Implementation Guide

### Board-Specific Touch Controllers

| Board | Controller | I2C Addr | SDA | SCL | INT | Display |
|-------|-----------|----------|-----|-----|-----|---------|
| T-Display S3 AMOLED Plus (1.91") | CST816T | 0x15 | GPIO 3 | GPIO 2 | GPIO 21 | 536×240 landscape |
| Waveshare ESP32-S3 1.8" AMOLED | FT3168 | 0x38 | GPIO 15 | GPIO 14 | GPIO 21 | 368×448 portrait |

**Key lesson:** Same I2C pins ≠ same touch controller. Always check vendor demo code for the actual driver being used.

Both controllers use **direct I2C register reads** — SensorLib was bypassed for reliability. See per-controller sections below for details.

### CST816 Direct I2C Implementation (T-Display S3 AMOLED Plus)

**History:** Originally used SensorLib's `TouchDrvCSTXXX` wrapper, which only returned the home button coordinate (600, 120) and never real touch points — likely due to auto-sleep mode or initialization issues that couldn't be debugged through the library abstraction. Bypassed SensorLib for direct I2C register reads (same proven approach as FT3168). Result: 14KB smaller binary, reliable touch.

**Critical Init Sequence (order matters):**

1. **INT pin wake-up (pseudo-reset):** The T-Display S3 AMOLED Plus has no dedicated RST pin. Driving GPIO 21 (INT) LOW for 50ms forces the controller out of sleep/gesture-only mode:
   ```cpp
   pinMode(TOUCH_INT, OUTPUT);
   digitalWrite(TOUCH_INT, LOW);
   delay(50);
   pinMode(TOUCH_INT, INPUT);
   delay(50);
   ```
   This MUST happen BEFORE `Wire.begin()`.

2. **I2C init:** `Wire.begin(TOUCH_SDA, TOUCH_SCL)` at 100kHz.

3. **Disable auto-sleep:** Write 0x01 to register 0xFE immediately after I2C init. The controller re-enters sleep within ~5s if this isn't done.

4. **Set interrupt mode:** Write 0x60 to register 0xFA to enable touch+change interrupts (not gesture-only mode).

**Register Map (CST816):**
- `0x00-0x06`: Touch status + coordinates (read 7 bytes in single transaction)
- `0x02`: Touch count (lower 4 bits)
- `0x03-0x04`: X coordinate (high nibble + low byte)
- `0x05-0x06`: Y coordinate (high nibble + low byte)
- `0xA7`: Chip ID (`0xB4`=CST816S, `0xB5`=CST816T, `0xB6`=CST816D, `0xB7`=CST820)
- `0xA9`: Firmware version
- `0xFE`: Auto-sleep disable (write `0x01`)
- `0xFA`: Interrupt mode (write `0x60` for touch+change)
- `0xE5`: Sleep control (write `0x03` to sleep)

**Home Button Filtering:** The CST816T reports a virtual home button at (600, 120) or swapped (120, 600). These must be filtered in `hal_touch_read()` before coordinate processing.

**Coordinate System:** The CST816T reports coordinates directly in display space — NO rotation, NO scaling, NO axis swap needed:
```cpp
transformed_x = raw_x;  // Direct pass-through
transformed_y = raw_y;  // No transform
```
Touchable area from HIL: X: 2-536, Y: 46-239 (slightly smaller than full 536×240 display).

**Why SensorLib failed:** Two issues:
1. SensorLib's `isPressed()` + `getPoint()` are two separate I2C transactions. The interrupt flag auto-clears after the first read, creating phantom RELEASE events that break the gesture state machine.
2. The library's initialization sequence didn't reliably wake the controller from sleep/gesture-only mode without a hardware RST pin.

### FT3168 Direct I2C Implementation (Waveshare ESP32-S3 1.8")

Simpler than CST816 — no wake-up sequence, no auto-sleep disable, no home button filtering needed.

**Register Map (FocalTech standard):**
- `0x02`: Number of touches (lower 4 bits)
- `0x03-0x06`: X/Y coordinates (same nibble+byte format as CST816)
- `0xA3`: Chip ID

**Coordinate System:** Portrait mode (0° rotation), direct pass-through. No transforms needed.

**Why not SensorLib:** FT3168 chip ID (0x03) isn't in SensorLib's FT6X36 driver allowlist — `initImpl()` rejects it. Direct I2C reads are ~20 lines of code with zero driver compatibility risk. The FocalTech register layout (0x02-0x06) is identical across FT3168/FT6X36/FT6206.

### Rate Limiter Anti-Pattern (CRITICAL)

**Never** use rate limiters that return `is_pressed = false` on skipped frames. The gesture engine's `update()` method transitions to `STATE_IDLE` on ANY frame where `is_pressed == false`, destroying sustained gesture detection (HOLD, SWIPE, DRAG).

The state machine sees: PRESS → IDLE → PRESS → IDLE (every N frames), making sustained gestures impossible.

If rate limiting is needed: return the LAST KNOWN touch state on skipped frames, or skip only the I2C read without changing the reported state.

### Touch Gesture Edge Zone Tuning

Edge zones determine whether a touch starts in the "center" (→ SWIPE) or at an "edge" (→ EDGE_DRAG). Getting these wrong causes center swipes to be misclassified as edge drags.

**T-Display S3 AMOLED Plus (536×240 landscape):**
```cpp
engine->setEdgeZones(
    40,   // left: x < 40 (7.5% of 536)
    430,  // right: x > 430 (80% — last 20%)
    36,   // top: y < 36 (15% of 240)
    204   // bottom: y > 204 (85% — last 15%)
);
```
This leaves ~51% of the screen as "center" for SWIPE detection.

**Waveshare ESP32-S3 1.8" (368×448 portrait):**
Uses default percentage-based thresholds (30% from each edge via `EDGE_THRESHOLD_PERCENT`). No custom configuration needed — full-screen touch panel.

**Tuning lessons learned:**
- Edge zones > 60% of screen leave almost no center area for SWIPE detection (the original zones gave only 10% center)
- RIGHT edge threshold must be reachable by actual finger touches on a 1.91" screen
- Axis-aware swipe thresholds (`getSwipeDistanceThreshold(dx, dy)`) prevent aspect ratio distortion on non-square screens — horizontal and vertical swipes use their respective axis dimension for threshold calculation

### Debugging Protocol for Touch Issues

1. **Check RAW register bytes:** Log the 7-byte I2C read to verify the controller returns real data (not stuck values like `00 AA 01 82 58 00 78` → always 600,120)
2. **Verify init sequence:** For CST816: INT pin wake → I2C init → auto-sleep disable → interrupt mode set. Missing any step can leave the controller in gesture-only mode
3. **Tap all 4 corners:** Compare expected vs actual coordinates to determine if rotation/scaling/swapping is needed. Both controllers on LPad use direct pass-through (no transforms)
4. **Log start positions for gestures:** Shows which zone (LEFT/RIGHT/TOP/BOTTOM/CENTER) triggered the classification
5. **Test with FT3168 first:** It's simpler — if gestures work there, the gesture engine is fine and the issue is CST816 HAL-level

## [2026-02-11] Waveshare ESP32-S3 1.8" AMOLED: Wrong Touch Controller, Rate Limiter Phantom Releases, and Title Clipping

### Challenge 1: FT3168 ≠ CST816 — Wrong Touch Controller Assumption

**Problem:** The `esp32s3` environment (Waveshare ESP32-S3 1.8" AMOLED Touch) was assumed to use a CST816 touch controller because the T-Display S3 AMOLED Plus uses one and the pin assignments (SDA=15, SCL=14, INT=21) are identical. Touch init was failing silently.

**Discovery:** Examining the vendor examples in `hw-examples/ESP32-S3-Touch-AMOLED-1.8-Demo/` revealed:
- The drawing board demo uses `Arduino_FT3x68` driver, NOT `TouchDrvCSTXXX`
- I2C address is **0x38** (FocalTech FT3168), not **0x15** (CST816)
- The `pin_config.h` confirmed identical I2C pins but completely different touch IC

**Solution:** Created `hal/touch_ft3168.cpp` using direct I2C register reads (FocalTech standard register layout: 0x02=num_touches, 0x03-0x06=X/Y coords).

**platformio.ini Impact:** Required adding `-<../hal/touch_ft3168.cpp>` or `-<../hal/touch_cst816.cpp>` exclusions to ALL 16 build environments to prevent duplicate symbol errors (both files implement `hal_touch_init`/`hal_touch_read`).

### Challenge 2: Rate Limiter Causing Phantom Release Events

**Problem:** After touch init worked correctly on the Waveshare board, coordinates registered fine but the gesture engine detected only TAP events — no HOLD, SWIPE, or DRAG. The finger-down state was being interrupted every few frames.

**Root Cause:** The rate limiter returned `is_pressed = false` on 2 out of every 3 frames, causing the gesture engine to see PRESS → IDLE → PRESS → IDLE on every 3 frames. See "Rate Limiter Anti-Pattern" in the Touch System guide above.

**Solution:** Removed the rate limiter entirely from `touch_ft3168.cpp`. The FT3168 handles full-speed I2C polling without issues.

### Challenge 3: getTextBounds() Clipping Last Character

**Problem:** The on-screen title displayed "DEMO v0.6" instead of "DEMO v0.65". The "5" was missing.

**Root Cause:** `Arduino_GFX::getTextBounds()` can undercount the width of the last character's advance by 1-2 pixels. The canvas was sized to exactly `w` pixels from `getTextBounds()`, clipping the final character.

**Solution:** Added 2 pixels of padding: `m_titleBufferWidth = w + 2;`

**Key Lesson:** **Never trust `getTextBounds()` for exact canvas sizing.** Always add 2-4 pixels of width padding when using the bounds to allocate a rendering canvas.

## [2026-02-11] TouchTestOverlay Crash: Arduino_Canvas Requires Valid Parent Device

### Problem
Release v0.65 crashed immediately upon detecting the first touch tap gesture:
```
Guru Meditation Error: Core  1 panic'ed (StoreProhibited). Exception was unhandled.
EXCVADDR: 0x00000000
```

### Root Cause Chain (3 crashes, 3 fixes)

**Crash 1: Null pointer dereference**
Canvas created with `nullptr` parent AND missing `begin()` call:
```cpp
Arduino_Canvas canvas(m_text_width, m_text_height, nullptr);  // No begin()!
canvas.fillScreen(CHROMA_KEY);  // CRASH: framebuffer not allocated
```
**Fix:** Call `canvas.begin(GFX_SKIP_OUTPUT_BEGIN)` to allocate the internal framebuffer.

**Crash 2: Stack overflow**
Canvas allocated on stack caused stack overflow during touch event handling:
```cpp
Arduino_Canvas canvas(...);  // STACK - too large for local variable
```
**Fix:** Heap allocate: `Arduino_Canvas* canvas = new Arduino_Canvas(...);`

**Crash 3: Double-free from repeated allocation**
Creating/destroying `Arduino_Canvas` on every touch event corrupted heap:
```
assert failed: tlsf_free tlsf.c:1120 (!block_is_free(block) && "block already marked as free")
```
**Fix:** Create canvas once as class member, reuse forever:
```cpp
class TouchTestOverlay {
    Arduino_Canvas* m_render_canvas;  // Created once in begin(), deleted in destructor
};
```

### Complete Arduino_Canvas Pattern
1. **Heap allocate:** `Arduino_Canvas* canvas = new Arduino_Canvas(width, height, gfx)`
2. **Initialize framebuffer:** `canvas->begin(GFX_SKIP_OUTPUT_BEGIN)`
3. **Reuse:** `canvas->fillScreen()`, `canvas->print()`, etc. — never recreate
4. **Clean up once:** `delete canvas;` in destructor only

**Never allocate large Arduino_Canvas objects on the stack. Never create/destroy canvases in render paths.**

## [2026-02-11] Touch Overlay Rendering Optimization

### Problem
Touch overlay blit happened every frame even when overlay content hadn't changed, wasting DMA bandwidth.

### Solution: Conditional Blit with Dirty Flag
Added `m_needs_blit` flag to track when overlay content changes:
```cpp
void TouchTestOverlay::update(const touch_gesture_event_t& event) {
    m_buffer_valid = false;  // Re-render text
    m_needs_blit = true;     // Mark for blit
}

void TouchTestOverlay::render() {
    if (!m_buffer_valid) {
        renderTextToBuffer();
        m_needs_blit = true;
    }
    if (!m_needs_blit) return;  // Skip if unchanged
    hal_display_fast_blit_transparent(...);
    m_needs_blit = false;
}
```

Also added `markForReblit()` to re-blit after graph renders that may overwrite the overlay.

**Impact:** Overlay blit only happens on new gesture (~1fps) or after graph update, eliminating ~29 unnecessary blits/sec at 30fps.

### Buffer Sizing
Buffer size 200x60 was too small for full gesture text ("EDGE_DRAG: RIGHT" + coordinates). Increased to 300x80 to accommodate worst-case text with 18pt font.

### Key Lessons
1. **Dirty flags prevent redundant render operations** — track when content actually changes
2. **Buffer size must accommodate worst-case content** — measure with actual font metrics
3. **DMA blits are fast but not free** — eliminate unnecessary blits even if each is only 1-2ms

## [2026-02-11] Release v0.60 Final: Y-Axis Tick Spacing & Label Uniqueness

### Problem
During v0.60 HIL testing, two critical graph rendering issues were discovered:
1. **Non-uniform Y-axis tick spacing**: Tick marks appeared at inconsistent pixel intervals (2, 2, 2, 3, 2, 2, 3 pixels) creating visible unevenness
2. **Duplicate Y-axis labels**: Tick increment (0.002 = 4 significant figures) was too fine for 3-sig-fig display, producing identical labels like "4.14", "4.14", "4.14"
3. **Missing X-axis "0" label**: The "NOW" (0 hours prior) label didn't always appear at the most recent data point

### Root Cause Analysis

**Issue 1: Tick Spacing**
The ticks were correctly generated at "clean" data values (4.134, 4.136, 4.138...) per feature spec requirement: "tick at 4.19 must be at exact vertical position representing 4.19". These values mapped to fractional pixel positions (78.316, 76.165, 74.013...) with uniform spacing of 2.152 pixels.

However, when `RelativeDisplay::relativeToAbsoluteY()` applied `roundf()` to convert to integer pixels:
- 78.316 → 78
- 76.165 → 76 (gap: 2 pixels)
- 74.013 → 74 (gap: 2 pixels)
- 71.861 → 72 (gap: 2 pixels) ← rounds up
- 69.709 → 70 (gap: 2 pixels)
- 67.557 → 68 (gap: 2 pixels)
- 65.405 → 65 (gap: 3 pixels!) ← accumulated rounding error

**Issue 2: Duplicate Labels**
The `format_3_sig_digits()` function produces 3-digit precision (e.g., "4.14"), but tick values at 0.002 increment require 4 digits to distinguish:
- 4.138 → "4.14"
- 4.140 → "4.14" ← duplicate!
- 4.142 → "4.14" ← duplicate!
- 4.144 → "4.14" ← duplicate!

The code had duplicate detection but only tried `tick_skip = 2`, which wasn't sufficient.

**Issue 3: X-axis "0" Missing**
The X-tick loop (`for i = tick_interval; i < num_points; i += tick_interval`) didn't guarantee the last data point would be included. If `num_points=100` and `tick_interval=20`, ticks appeared at i=20, 40, 60, 80 — missing the last point (99).

### Discarded Approach: Uniform Pixel Spacing (WRONG)

**Attempt:** Calculate integer pixel spacing, place ticks at evenly-spaced pixel positions, map back to data values for labels.

**Why it failed:**
1. **Violated feature spec**: "Tick at 4.19 must be at exact vertical position representing 4.19" — this approach placed ticks at pixel positions that didn't correspond to their labeled data values
2. **Broke origin suppression**: The algorithm recalculated all tick positions, bypassing the origin suppression check that prevents ticks from overlapping the X-axis
3. **Regression introduced**: A Y-tick was drawn over the X-axis, making the issue worse

**Key insight from user:** "We are only displaying 3 significant digits (e.g., 4.19), but the value is being represented as 4.192, 4.194, 4.196... 4 significant digits)" — the real problem wasn't pixel spacing, it was tick density relative to display precision.

### Final Solution: Iterative Tick Skip Adjustment

Instead of fighting the pixel rounding, we **increase tick density** until all labels are unique:

```cpp
bool has_duplicates = true;
while (has_duplicates && tick_skip < static_cast<int>(all_ticks.size())) {
    // Generate labels for ticks at current skip level
    // Check for duplicates
    if (has_duplicates) tick_skip++;
}
```

**For the example data (range 0.079, increment 0.002):**
- tick_skip=1: 35 ticks, many duplicates
- tick_skip=2: 17 ticks, still duplicates
- tick_skip=5: 7 ticks with labels "4.13", "4.14", "4.15", "4.16", "4.17", "4.18" → ALL UNIQUE

**X-axis fix:** Always include last data point in tick indices.

### Key Lessons
1. **Don't fight the spec to fix a symptom** - The spec requires ticks at exact data values; trying to move them to uniform pixels violates this requirement
2. **Precision mismatch is solvable by density** - If display precision is coarser than data increment, reduce tick count until labels are distinguishable
3. **Iterative algorithms > fixed heuristics** - `tick_skip = 2` hardcoded guess fails; `while (has_duplicates) tick_skip++` always succeeds
4. **Feature specs can conflict** - "Ticks at exact clean values" + "Uniform visual spacing" are mathematically incompatible when increment is fractional pixels; prioritize data accuracy over visual perfection

---

## [2026-02-09] Release v0.60 HIL Test Round 2: Rendering Optimization & Aspect Ratio Fix

### Problems
After initial v0.60 HIL testing, three issues were reported:
1. **DEMO text too large**: "DEMO v0.60" text used 18pt font and was positioned 10px from corner - should be smaller and flush to corner
2. **Flashing/slow animation**: DEMO text, mini logo, and live indicator were flashing; animation felt slow
3. **Mini logo vertically squashed**: Logo appeared compressed (too short) compared to final frame of LogoScreen animation

### Root Causes
1. **Text size**: Used `theme->fonts.ui` (18pt) instead of smallest font; 10px offset pushed it away from corner
2. **Unnecessary redraws**: Render loop called `m_graph->render()`, `m_miniLogo->render()`, and `blitTitle()` every frame (30fps), even when data hadn't changed. This caused visible flashing and performance issues.
3. **VectorRenderer aspect ratio bug**: VectorRenderer calculated `target_height = width_percent * shape_aspect_ratio` but didn't account for screen aspect ratio. Since width_percent is % of screen width and target_height is % of screen height, the calculation needed to multiply by `screen_width / screen_height` to convert between the two coordinate spaces.

### Solutions
1. **Smaller text**: Changed from `theme->fonts.ui` (18pt) to `theme->fonts.smallest` (9pt); changed offset from 10px to 0px for flush top-left positioning
2. **Render optimization**:
   - Added `m_graphInitialRenderDone` flag to track if graph has been rendered
   - Changed render loop to only do full render (graph + mini logo + title) when:
     - Initial render needed (`!m_graphInitialRenderDone`)
     - Data actually changed (data length increased)
   - Live indicator animation (`m_graph->update()`) still runs every frame but uses efficient dirty-rect updates
3. **Aspect ratio fix**: Updated VectorRenderer to account for screen aspect ratio:
   ```cpp
   float screen_aspect_ratio = screen_width / screen_height;
   float target_height = width_percent * shape_aspect_ratio * screen_aspect_ratio;
   ```
   This correctly converts from width percentage (% of screen width) to height percentage (% of screen height) while maintaining the shape's aspect ratio.

### Key Lessons
1. **Use smallest font for compact overlays**: Status text like "DEMO v0.60" should use 9pt font, not 18pt UI font
2. **Optimize render loops**: Only redraw when data changes. Use dirty-rect updates for animations.
3. **VectorRenderer requires screen aspect ratio**: When calculating dimensions, always account for screen aspect ratio to prevent distortion on non-square displays
4. **Coordinate space matters**: Percentage-based coordinates must account for screen dimensions when converting between width% and height%

## [2026-02-09] Release v0.60 HIL Test Round 1: Theme Integration & Transparency

### Problem
During HIL testing of Release v0.60, multiple visual issues were discovered:
1. Theme colors not being used (logo background wrong, graph colors wrong)
2. Mini logo size mismatch (12% vs 10% height)
3. Title text flashing with black background instead of transparent overlay
4. Missing tick marks and labels on graph
5. Graph not updating when StockTracker receives new data

### Root Causes
1. **Theme colors**: V060DemoApp hardcoded black (0x0000) instead of using ThemeManager
2. **Mini logo size**: MiniLogo header defined 12% height vs LogoScreen's 10% end size
3. **Title transparency**: Used `hal_display_fast_blit()` with black background instead of `hal_display_fast_blit_transparent()` with chroma key 0x0001
4. **Graph ticks**: Missing `setYTicks()` call and not using theme colors properly
5. **Graph updates**: Render loop didn't check for data changes from StockTracker background task

### Solutions
1. **Theme integration**: Get theme from ThemeManager in `begin()` and use `theme->colors.background` for logo screen and graph
2. **Size consistency**: Changed MiniLogo `LOGO_HEIGHT_PERCENT` from 12.0f to 10.0f to match LogoScreen end size
3. **Transparent title**: Changed `renderTitleToBuffer()` to fill with chroma key 0x0001 and `blitTitle()` to use `hal_display_fast_blit_transparent()`
4. **Graph configuration**: Added `setYTicks(0.002f)` call and updated theme to use ThemeManager colors
5. **Dynamic updates**: Added static tracking of last data length in render loop, triggers `setData()` + `drawData()` when data changes

### Key Implementation Pattern: Transparent Overlay with Chroma Key
```cpp
constexpr uint16_t CHROMA_KEY = 0x0001;
canvas->fillScreen(CHROMA_KEY);  // Transparent background
canvas->setTextColor(RGB565_WHITE);
canvas->print(titleText);
hal_display_fast_blit_transparent(x, y, w, h, buffer, CHROMA_KEY);
```

### Key Lessons
1. **Always use ThemeManager**: Don't hardcode colors - themes provide consistency across all components
2. **Match component sizes**: When one component transitions to another (LogoScreen → MiniLogo), sizes must match exactly
3. **Chroma key for overlays**: Text/UI elements drawn on top of animated content require transparent blitting to prevent black backgrounds
4. **Graph needs explicit tick configuration**: Unlike v0.5 which set ticks in theme factory, v0.6 needs explicit `setYTicks()` call
5. **Background tasks require polling**: StockTracker runs autonomously - render loop must check for changes and trigger updates

## [2026-02-07] TE Sync Eliminates Tearing on T-Display S3 AMOLED Plus

### Problem
Subtle flickering/tearing artifacts visible during 30fps animations on T-Display S3 AMOLED Plus. The artifact appeared as a horizontal shimmer moving from top to bottom of the screen, even with consistent backgrounds.

### Root Cause
The RM67162 display driver was configured to enable the TE (Tearing Effect) pin during initialization (`0x35, 0x00`), but the ESP32-S3 code never read this signal. The display was signaling safe update periods (vertical blanking), but the code was pushing frames at arbitrary times.

**Timing Mismatch:**
- Animation frame rate: 30fps (33.3ms per frame)
- Display refresh rate: ~60Hz (16.6ms per frame)
- Without synchronization: Beat frequency creates visible tearing when DMA blits occur mid-refresh

### Solution: TE Pin Synchronization
Added `waitForTeSignal()` function that waits for LOW→HIGH transition on GPIO 9 (LCD_TE) before frame updates:

```cpp
static void waitForTeSignal(void) {
    // Wait for LOW (display actively scanning)
    while (digitalRead(LCD_TE) == HIGH) { delayMicroseconds(1); }

    // Wait for HIGH (vertical blanking begins)
    while (digitalRead(LCD_TE) == LOW) { delayMicroseconds(1); }

    // Safe to update display
}
```

**Modified Functions:**
- `hal_display_fast_blit()` - waits for TE before DMA blit
- `hal_display_fast_blit_transparent()` - waits for TE before transparent blit

**Impact:**
- All frame updates now occur during vertical blanking period
- Eliminates tearing artifacts completely
- Minimal performance impact (typically <1ms wait per frame at 30fps)

### Key Lessons
1. **TE pin configuration ≠ TE pin usage** - Hardware initialization enables the signal, but software must actively read it
2. **Tearing vs PWM flicker** - Top-to-bottom shimmer indicates timing mismatch, not PWM dimming
3. **Beat frequencies** - When animation FPS doesn't match display refresh rate, synchronization becomes critical
4. **Timeout protection** - TE sync loops include 10ms timeout to prevent infinite hangs if signal fails
5. **Hardware limits are real** - Even with perfect TE sync, AMOLED pixel response time (~2-5ms) and SPI transfer duration (~2-3ms for full screen) create inherent artifacts. "Very slight" residual flicker is the physical limit of the display technology.

---

## [2026-02-06] Title Text Rendering on Live Graph Updates (v0.58)

### Problem
In v0.58 demo, the "DEMO v0.58" title text needs to persist on screen while the graph updates every 3 seconds with live data. The graph uses full-screen DMA blit (`hal_display_fast_blit`) which overwrites everything including the title. Initial attempts caused visible flashing or complete disappearance of the title.

### Hardware Context
- **ESP32-S3 AMOLED & T-Display S3 Plus**: `hal_display_flush()` is a **no-op** - all drawing operations (Arduino_GFX and DMA blits) write directly to display hardware without framebuffer buffering
- **Graph rendering**: Uses `hal_display_fast_blit()` for direct DMA transfer from PSRAM composite buffer to display
- **Live indicator**: Uses partial DMA blits every frame to animate the pulsing dot

### Discarded Attempts

**Attempt 1: Font Rendering After Graph Blit**
- **Result:** Visible flash between graph blit and title appearing (~10-20ms gap)

**Attempt 2: Pre-draw Title to Framebuffer**
- **Result:** Title immediately overwritten, never restored (flush is no-op)

**Attempt 3: Title Buffer with Parent GFX**
- **Result:** `ESP_ERR_INVALID_STATE: SPI bus already initialized` crash

### Final Solution: Pre-rendered Buffer + Transparent DMA Blit

1. **One-time rendering**: Create standalone `Arduino_Canvas` with `nullptr` parent, fill with chroma key `0x0001`, render title text, copy framebuffer to cached buffer
2. **Fast blit**: Uses `hal_display_fast_blit_transparent()` with chroma key (~1-2ms DMA instead of ~10-20ms font rendering)
3. **Dual blit strategy**: Immediately after `graph->render()` + every frame in render loop

### Key Lessons
1. **hal_display_flush() behavior varies by hardware** - Always check HAL implementation, never assume buffering exists
2. **Arduino_Canvas parent parameter** - Use `nullptr` for standalone canvases to avoid SPI reinit
3. **Chroma key transparency** - Use `0x0001` for consistent transparency
4. **Buffer caching eliminates font rendering overhead** - One-time render + fast blit beats per-frame font rendering

---

## [2026-02-04] The "Flicker-Free" Graph Rendering Struggle

### Problem
When rendering the time-series graph, updating the "live indicator" (the pulsing dot) caused the entire graph line or background to flicker, or left "ghost" artifacts behind.

### Discarded Attempts
1.  **Full Redraw:** Clearing the screen and redrawing the entire graph every frame.
    *   *Result:* Unacceptable flicker at 30fps. The ESP32-S3 cannot push full 320x170 frames over SPI fast enough to beat the refresh rate.
2.  **XOR Drawing:** Attempting to "undraw" the previous dot using XOR logic.
    *   *Result:* Colors looked wrong on the gradient background.
3.  **Single Buffer w/ Invalidation:** Drawing directly to the screen and trying to "erase" just the dot by drawing a black rectangle over it.
    *   *Result:* Erased the graph line and background gradient behind the dot, creating "black holes."

### Final Solution: Layered Canvas + Partial Restore
We adopted a "Layered Rendering" strategy similar to game engines:
1.  **Background Canvas:** We allocate a PSRAM buffer for the static background (gradients + grid). This is drawn *once*.
2.  **Data Canvas:** We allocate a PSRAM buffer for the data line. This is drawn *once*.
3.  **Composition:** On `update()`, we:
    *   Calculate the bounding box of the *previous* dot frame.
    *   Blit the "clean" pixels from the Background and Data canvases *only* into that bounding box (effectively "restoring" the background).
    *   Draw the *new* dot at the new size/position.
4.  **Hardware:** This relies heavily on `hal_display_fast_blit` which uses DMA. Without DMA, this composition would be too slow.

### Key Lesson
**PSRAM is cheap; Bandwidth is expensive.** It is better to burn 150KB of PSRAM to store cached layers than to try to re-compute pixels or push full frames over the SPI bus.

---

## [2026-02-05] Standalone LiveIndicator vs Integrated Implementation

### Problem
When implementing the standalone `LiveIndicator` component (features/ui_live_indicator.md) and using it in the Base UI Demo Application (features/app_demo_screen.md), the indicator exhibits flashing artifacts during the 30fps pulsing animation.

### Root Cause
The standalone `LiveIndicator::draw()` method draws a radial gradient circle directly to the display using `display_relative_fill_circle_gradient()`. Without dirty-rect optimization, the calling code must either:
1. Redraw the entire graph before each indicator frame (causing flicker)
2. Track the previous indicator position and manually restore that region

### Decision for v0.5 Demo
Initially attempted standalone LiveIndicator with application-level dirty-rect, but flashing was unacceptable for HIL testing.

**Final Implementation:** Uses integrated TimeSeriesGraph indicator:
- Flicker-free 30fps animation
- Dirty-rect optimization with composite buffer restoration
- Production-ready implementation

The standalone `LiveIndicator` component remains fully implemented and unit-tested (5 tests passing), available for use cases where dirty-rect can be managed by the caller.

**Conclusion:** For SPI-based displays where bandwidth is limited, integrated components with tight coupling to the rendering pipeline are the pragmatic choice over pure component separation.

### Key Lessons
1. **Dirty-rect optimization is not optional** for smooth animation on SPI displays
2. **Component reusability vs performance** is a real trade-off - sometimes tight coupling is the right choice
3. **TimeSeriesGraph's integrated indicator** should be the reference implementation for any future dirty-rect work

---

## [2026-02-05] PlatformIO Demo Directory Include Paths

### Problem
Demo files in `demos/` directory failed to compile/run with no serial output or screen activity. Using relative includes like `#include "../src/file.h"` and `#include "../hal/file.h"`.

### Root Cause
PlatformIO automatically adds `src/` and `hal/` (via `-Ihal` build flag) to the include search path. Relative paths from `demos/` resolved incorrectly during compilation.

### Solution
Use direct includes: `#include "file.h"` instead of `#include "../src/file.h"`. PlatformIO's include path configuration handles the rest.

### Key Lesson
**Trust PlatformIO's conventions.** The build system is designed for `src/` as the primary source directory. Additional directories should use direct includes, relying on `-I` flags in `platformio.ini`.

---

## [2026-02-06] Animated LogoScreen with Dirty-Rect Optimization

### Problem
The initial LogoScreen implementation drew the logo directly to the display using VectorRenderer each frame, causing visible refresh artifacts during the smooth EaseInOut animation (logo moving from center to top-right corner while shrinking).

### Solution: Dirty-Rect with Composite Buffer
Applied the same technique used in TimeSeriesGraph's live indicator:

1. **Background Composite Buffer (PSRAM):** Allocated full-screen buffer storing clean background
2. **Dirty Rect Calculation:** Calculate union of old and new logo bounding boxes
3. **Three-Step Atomic Update:**
   - Copy clean background from composite to temp buffer (erases old logo)
   - Rasterize new logo into temp buffer using barycentric coordinates
   - Single atomic blit to display via `hal_display_fast_blit`

### Updated Animation Parameters (2026-02-06)
Re-implemented LogoScreen with corrected animation parameters per feature spec:
- **End size**: 10% of screen height
- **End anchor point**: Top-right corner of the logo image (1.0, 0.0)
- **End position**: The anchor point (top-right of logo) is placed 10px from screen edges

**Critical clarification**: The anchor point (1.0, 0.0) means the top-right corner OF THE LOGO IMAGE, not the screen. This anchor point is positioned at the calculated screen coordinates, causing the rest of the logo to extend leftward and downward from that point.

### Key Lessons
1. **Same solution for same problem**: Animated vector graphics require the same dirty-rect optimization as animated raster graphics
2. **Software rasterization trade-off**: Slight CPU cost (barycentric test per pixel) for smooth animation via atomic blit
3. **PSRAM allocation strategy**: Composite buffer allocation uses `ps_malloc` on BOARD_HAS_PSRAM for optimal performance
4. **Resolution independence**: Animation parameters should be calculated dynamically based on actual screen dimensions, not hardcoded for specific resolutions

---

## [2026-02-05] Mode-Switching Demo for Comprehensive Visual Testing

### Problem
The demo_release_0.5.md feature spec explicitly requires testing BOTH gradient and solid rendering modes for all UI components (background, graph line, live indicator). The initial implementation only demonstrated gradient mode, leaving solid mode untested during HIL validation.

### Solution
Implemented an automatic mode-switching demo that cycles through three visual styles every 8 seconds:
- **Mode 0 (ALL GRADIENTS)**: 45° background gradient, gradient plot line (cyan→pink), gradient indicator (magenta→cyan)
- **Mode 1 (ALL SOLID)**: Solid dark grey background, solid white plot line, solid green indicator
- **Mode 2 (MIXED)**: Solid dark blue background, gradient plot line (yellow→red), gradient indicator (yellow→red)

This approach required adding a `setTheme()` method to the `TimeSeriesGraph` class to enable runtime theme changes without recreating the entire graph object.

### Key Lessons
1. **Automated cycling is better than manual switching** for HIL testing - the user can simply watch the display and verify all modes work correctly without manual intervention
2. **Runtime theme switching requires careful layer management** - static layers (background, data) must be explicitly redrawn when the theme changes, while dynamic layers (live indicator) continue updating normally
3. **Demo comprehensiveness directly impacts feature validation quality** - testing both code paths (gradient vs solid) in a single demo run reduces the risk of undiscovered bugs in less-exercised rendering modes

---

## [2026-02-06] Domain-Segmented HAL & Interactive Graph Viewer

### Problem
As the project expands into Networking and HTTP (v0.6), the monolithic `hal_contracts.md` and the terminal-based dependency graph visualization were becoming difficult to maintain and read.

### Solution: Domain-Segmented HAL
1.  **HAL Core Contract**: Established a root contract (`hal_core_contract.md`) defining fundamental rules (C-compatibility, Error Handling, Stub Mandate).
2.  **Modular Specifications**: Split the monolithic contract into domain-specific specs:
    *   `hal_spec_display.md`
    *   `hal_spec_timer.md`
    *   `hal_spec_network.md` (New for v0.6)
3.  **Encapsulation**: Each domain now has its own header (e.g., `hal/network.h`, `hal/display.h`) to prevent application logic from needing to include unrelated hardware definitions.

### Solution: Interactive Graph Viewer
1.  **Problem**: The iTerm2 terminal image was not zoomable, making large graphs unreadable.
2.  **Interactive Viewer**: Created `scripts/serve_graph.py` (minimal Python server) and `scripts/graph_viewer.html` (Mermaid.js + svg-pan-zoom).
3.  **Live-Reload**: The viewer polls for changes to `feature_graph.mmd` and re-renders in-place, preserving zoom level.

### Key Lessons
1.  **Architecture is a living document**: Don't be afraid to refactor the HAL *before* it gets too messy. Segmenting by domain early prevents "Header Spaghetti."
2.  **Browser > Terminal for Visualization**: Browsers offer superior interactive capabilities (zoom, search, pan) for complex diagrams.
3.  **Gherkin Prerequisite Formatting**: Discovered that the dependency parser is sensitive to formatting. Multiple prerequisites MUST be on separate lines.

### Problem
The demo_release_0.5.md spec requires two layout modes: "Scientific" (OUTSIDE tick labels + axis titles) and "Compact" (INSIDE tick labels, no titles). Previous attempts to modify the graph draw methods caused watchdog reset (TG1WDT_SYS_RST) crashes.

### Root Cause of Previous Crashes
Including `theme_manager.h` directly in `ui_time_series_graph.cpp` pulled in all 5 font data arrays (~100KB+ of static data). Combined with the draw method modifications, this likely caused memory pressure or stack overflow during the pixel-intensive background rendering.

### Solution: Font Injection via GraphTheme Struct
Instead of the graph code accessing `ThemeManager::getInstance()` directly, fonts are passed through the `GraphTheme` struct:
- `tickFont`: For tick labels (9pt smallest)
- `axisTitleFont`: For axis titles (18pt UI)

The demo code (which already includes `theme_manager.h`) sets these when creating themes.

### Key Lessons
1. **Never include font headers in draw code** - pass font pointers through configuration structs instead
2. **Watchdog crashes on ESP32-S3** are often caused by memory pressure during pixel-intensive operations, not necessarily by timing
3. **Incremental modification of draw methods** after full revert is safer than trying to fix crashes in-place
4. **CRITICAL: Custom GFX fonts crash on PSRAM Arduino_Canvas** - calling `canvas->setFont(GFXfont*)` on an Arduino_Canvas allocated in PSRAM causes TG1WDT_SYS_RST watchdog resets. **Workaround:** Use built-in font with `canvas->setFont(nullptr); canvas->setTextSize(N);` instead.

---

## [2026-02-06] V0.58 Demo: Live Data Updates & Rendering Synchronization

### Overview
Release v0.58 introduces dynamic, self-updating data using `DataItemTimeSeries` FIFO ring buffer. The demo simulates real-time data by injecting random values every 3 seconds while the graph actively displays, creating a scrolling effect as old data falls out and new data appears.

### Key Architectural Decisions

#### 1. Embedded Test Data Instead of Filesystem
**Problem:** Initial implementation used LittleFS filesystem to store test data, requiring separate filesystem upload.
**Solution:** Created `test_data/test_data_tnx_5m.h` with embedded C++ arrays (15 timestamps + 15 prices).
**Benefit:** Simpler deployment (single firmware upload), no filesystem dependencies, data baked into binary.

#### 2. DataItemTimeSeries Sizing for Sliding Window
**Problem:** Initial max_length was 50, causing graph to compress horizontally as it accumulated 16, 17... 50 points before FIFO kicked in.
**Solution:** Set max_length to `TestData::TNX_5M_COUNT` (15), matching initial test data size.
**Result:** Graph maintains constant 15-point window, oldest data evicted as new data arrives (true FIFO behavior).

#### 3. Fixed Y-Bounds to Prevent Data Drift
**Problem:** Random data generated from current min/max created feedback loop - if random values trended low, next iteration's range was lower, eventually drifting toward zero.
**Solution:** Capture initial Y-bounds (4.269 - 4.279 from test data) in `m_initialYMin/m_initialYMax`, use these fixed bounds for all random generation.
**Result:** Data oscillates within stable range indefinitely, Y-axis labels remain consistent.

### Rendering Challenges & Solutions

#### Challenge 1: Graph Overwriting Logo Screen
**Problem:** Graph updates happened during logo animation phase, overwriting the logo.
**Solution:** Added granular stage checking:
- V055 Phase check: `isInVisualPhase()` (not connectivity/handover)
- V05 Stage check: `isShowingGraph()` (STAGE_GRAPH_CYCLE, not STAGE_LOGO)
- Combined: `v055Demo->isInVisualPhase() && v05Demo->isShowingGraph()`

#### Challenge 2: Live Indicator Misalignment
**Problem:** After data updates, live indicator didn't track the last data point on the graph.
**Solution:** Call `graph->render()` in `updateGraphWithLiveData()` after `drawData()` to composite and blit updated data canvas.

### Implementation Pattern: Wrapper Composition

V058DemoApp follows the established wrapper pattern:
```
V058DemoApp (Live Data)
  └─ wraps V055DemoApp (WiFi + Connectivity)
      └─ wraps V05DemoApp (Logo + 6 Graph Modes)
```

### Key Lessons
1. **Embedded data > Filesystem for demos** - Simpler deployment, no upload dependencies
2. **FIFO sizing must match use case** - Set max_length to match expected window size, not arbitrary buffer
3. **Fixed bounds prevent drift** - Store initial data range for random generation, don't use dynamic min/max
4. **Granular phase checking required** - Single-level phase checks insufficient when phases have sub-stages
5. **DMA vs Framebuffer sync is hard** - Direct hardware blitting (DMA) and framebuffer rendering are fundamentally async, perfect sync may not be achievable without architectural changes
