# Implementation Log & Lessons Learned

This document captures the "tribal knowledge" of the project: technical hurdles, why specific decisions were made, and what approaches were discarded.

## [2026-02-11] TouchTestOverlay Crash: Arduino_Canvas Requires Valid Parent Device

### Problem
Release v0.65 crashed immediately upon detecting the first touch tap gesture:
```
[Touch] TAP at (120, 0) = (22.4%, 0.0%)
Guru Meditation Error: Core  1 panic'ed (StoreProhibited). Exception was unhandled.
EXCVADDR: 0x00000000
```

A `StoreProhibited` exception (0x1d) indicated a null pointer dereference in the TouchTestOverlay's render path.

### Root Cause
In `ui_touch_test_overlay.cpp:99`, the temporary canvas was created with `nullptr` as the parent device AND the critical `begin()` initialization was missing:
```cpp
Arduino_Canvas canvas(m_text_width, m_text_height, nullptr);  // TWO BUGS
canvas.fillScreen(CHROMA_KEY);  // CRASH: framebuffer not allocated
```

**Bug 1:** Passing `nullptr` as parent device (though after testing, this alone might not crash)
**Bug 2 (PRIMARY):** Missing `canvas.begin(GFX_SKIP_OUTPUT_BEGIN)` call — **this allocates the internal framebuffer**. Without it, all drawing operations dereference a null framebuffer pointer.

### Solution
Always provide a valid parent display device AND call `begin()` before any drawing operations:
```cpp
Arduino_GFX* gfx = static_cast<Arduino_GFX*>(hal_display_get_gfx());
Arduino_Canvas canvas(m_text_width, m_text_height, gfx);

// CRITICAL: Initialize canvas framebuffer before drawing
if (!canvas.begin(GFX_SKIP_OUTPUT_BEGIN)) {
    Serial.println("[ERROR] Canvas begin() failed");
    return;
}

canvas.fillScreen(CHROMA_KEY);  // NOW SAFE
```

The HAL provides `hal_display_get_gfx()` for the parent device. See `hal/display_tdisplay_s3_plus.cpp:278` and `ui_time_series_graph.cpp:118` for reference patterns.

### Lesson Learned: Stack vs Heap Allocation (CRITICAL)
After fixing the null pointer crash, a **second crash occurred** - "Double exception" with corrupted backtrace. This indicated **stack overflow**.

**Root Cause (Secondary):** The canvas was initially allocated on the stack:
```cpp
Arduino_Canvas canvas(m_text_width, m_text_height, gfx);  // STACK - WRONG for large canvases
```

Even with the framebuffer heap-allocated by `begin()`, the `Arduino_Canvas` object itself is large enough to cause stack overflow when created as a local variable, especially during interrupt-driven touch event handling.

**Final Solution:**
```cpp
// Allocate canvas on HEAP, not stack
Arduino_Canvas* canvas = new Arduino_Canvas(m_text_width, m_text_height, gfx);
if (canvas == nullptr || !canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) {
    delete canvas;
    return;
}

// Use canvas...

delete canvas;  // Clean up when done
```

This pattern is used throughout the codebase (see `v060_demo_app.cpp:407`, HAL canvas functions) and MUST be followed for all temporary canvas creation.

### Complete Arduino_Canvas Initialization Pattern
1. **Heap allocate:** `Arduino_Canvas* canvas = new Arduino_Canvas(width, height, gfx)`
2. **Initialize framebuffer:** `canvas->begin(GFX_SKIP_OUTPUT_BEGIN)`
3. **Use canvas:** `canvas->fillScreen()`, `canvas->print()`, etc.
4. **Clean up:** `delete canvas;`

**Never allocate large Arduino_Canvas objects on the stack** — always use heap allocation to prevent stack overflow crashes.

### Third Crash: Double-Free from Repeated Canvas Allocation (CRITICAL)
After fixing stack overflow, a **third crash occurred** - double-free assertion in memory allocator:
```
assert failed: tlsf_free tlsf.c:1120 (!block_is_free(block) && "block already marked as free")
```

**Root Cause:** Creating and destroying `Arduino_Canvas` on EVERY touch event (multiple times per second) caused heap corruption:
```cpp
void renderTextToBuffer() {
    Arduino_Canvas* canvas = new Arduino_Canvas(...);  // Create
    canvas->begin();
    // ... use canvas ...
    delete canvas;  // Delete
}  // Called repeatedly - causes heap corruption!
```

The rapid allocation/deallocation cycle corrupted the heap allocator's metadata, leading to double-free errors on the 2nd-3rd touch event.

**Final Solution: Create Canvas Once, Reuse Forever**
```cpp
class TouchTestOverlay {
    Arduino_Canvas* m_render_canvas;  // Member variable, not temporary
};

bool begin() {
    m_render_canvas = new Arduino_Canvas(...);
    m_render_canvas->begin();
    // Canvas lives for lifetime of overlay
}

~TouchTestOverlay() {
    delete m_render_canvas;  // Clean up once at end
}

void renderTextToBuffer() {
    // REUSE existing canvas, don't create/destroy
    m_render_canvas->fillScreen(CHROMA_KEY);
    // ... render ...
}
```

**Critical Lesson:** For UI components that render frequently (touch overlays, animations, etc.), **NEVER create/destroy canvases in the render path**. Create them once during initialization and reuse them. This pattern is standard in the codebase (see `ui_time_series_graph.cpp` - canvases are members, not temporaries).

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
    std::vector<std::string> test_labels;
    for (size_t idx = 0; idx < all_ticks.size(); idx += tick_skip) {
        char label[16];
        format_3_sig_digits(all_ticks[idx].first, label, sizeof(label));
        test_labels.push_back(std::string(label));
    }

    // Check for duplicates
    has_duplicates = false;
    for (size_t i = 0; i < test_labels.size(); i++) {
        for (size_t j = i + 1; j < test_labels.size(); j++) {
            if (test_labels[i] == test_labels[j]) {
                has_duplicates = true;
                break;
            }
        }
        if (has_duplicates) break;
    }

    if (has_duplicates) tick_skip++;
}
```

**For the example data (range 0.079, increment 0.002):**
- tick_skip=1: 35 ticks, many duplicates
- tick_skip=2: 17 ticks, still duplicates
- tick_skip=5: 7 ticks with labels "4.13", "4.14", "4.15", "4.16", "4.17", "4.18" → ALL UNIQUE ✓

**X-axis fix:**
```cpp
std::vector<size_t> tick_indices;
for (size_t i = tick_interval; i < num_points; i += tick_interval) {
    tick_indices.push_back(i);
}

// Always include last data point
size_t last_index = num_points - 1;
if (tick_indices.empty() || tick_indices.back() != last_index) {
    tick_indices.push_back(last_index);
}
```

### Results
- ✓ Y-ticks remain at exact vertical positions for clean data values (spec compliant)
- ✓ All Y-tick labels are unique (auto-adjusted tick_skip from 1→5)
- ✓ No ticks overlap X-axis (origin suppression preserved)
- ✓ X-axis "0" label always appears at most recent data point
- ✓ Tick spacing is mathematically uniform (2.152 pixels), visual variation from rounding is acceptable given larger tick density reduces total variation

### Key Lessons
1. **Don't fight the spec to fix a symptom** - The spec requires ticks at exact data values; trying to move them to uniform pixels violates this requirement
2. **Precision mismatch is solvable by density** - If display precision is coarser than data increment, reduce tick count until labels are distinguishable
3. **User insight is invaluable** - "4 sig figs being displayed as 3 sig figs" immediately identified the root cause
4. **Iterative algorithms > fixed heuristics** - `tick_skip = 2` hardcoded guess fails; `while (has_duplicates) tick_skip++` always succeeds
5. **Feature specs can conflict** - "Ticks at exact clean values" + "Uniform visual spacing" are mathematically incompatible when increment is fractional pixels; prioritize data accuracy over visual perfection
6. **Test what users see, not what code calculates** - Debug showed "delta = -2.152" (uniform), but users saw "inconsistent spacing" (rounding artifacts); both were correct from their reference frames

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

### Key Technical Insight: Percentage Coordinate Space Conversion
VectorRenderer works in RelativeDisplay's percentage coordinate system where:
- X coordinates are 0-100% of screen WIDTH
- Y coordinates are 0-100% of screen HEIGHT

When converting a width percentage to height percentage to maintain aspect ratio:
1. Shape aspect = height / width (e.g., 370/245 = 1.51 for portrait logo)
2. Screen aspect = width / height (e.g., 536/240 = 2.23 for landscape display)
3. Height % = Width % × Shape aspect × Screen aspect

Without the screen aspect correction, shapes appear squashed or stretched because the percentage units don't account for non-square screens.

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
This is the established pattern from v0.58 (see IMPLEMENTATION_LOG line 88):
```cpp
// Render title to buffer with chroma key
constexpr uint16_t CHROMA_KEY = 0x0001;
canvas->fillScreen(CHROMA_KEY);  // Transparent background
canvas->setTextColor(RGB565_WHITE);
canvas->print(titleText);

// Fast blit with transparency
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
```cpp
graph->render();        // Full-screen DMA blit (title disappears)
drawTitle();            // Font rendering with Arduino_GFX (slow)
hal_display_flush();    // No-op
```
- **Result:** Visible flash between graph blit and title appearing
- **Root cause:** `drawTitle()` involves expensive operations (getTextBounds, font rendering, print) creating ~10-20ms gap

**Attempt 2: Pre-draw Title to Framebuffer (WRONG)**
```cpp
drawTitle();            // Writes to display
graph->render();        // DMA blit overwrites title
hal_display_flush();    // No-op
```
- **Result:** Title immediately overwritten, never restored
- **Root cause:** Misunderstanding of flush behavior - since flush is no-op, title drawn before graph render is immediately lost

**Attempt 3: Title Buffer with Parent GFX (CRASH)**
```cpp
Arduino_Canvas* canvas = new Arduino_Canvas(width, height, gfx);  // gfx = Arduino_ESP32QSPI
```
- **Result:** `ESP_ERR_INVALID_STATE: SPI bus already initialized` crash
- **Root cause:** Passing hardware GFX object as canvas parent triggers SPI bus reinitialization

### Final Solution: Pre-rendered Buffer + Transparent DMA Blit

**Architecture:**
1. **One-time rendering**: `V05DemoApp::renderTitleToBuffer()`
   - Create standalone `Arduino_Canvas` with `nullptr` parent (avoids SPI reinit)
   - Fill with chroma key `0x0001` for transparency
   - Render title text using Arduino_GFX font API
   - Copy framebuffer to cached buffer

2. **Fast blit**: `V05DemoApp::blitTitle()`
   - Uses `hal_display_fast_blit_transparent()` with chroma key
   - DMA operation (~1-2ms) instead of font rendering (~10-20ms)
   - Pixels matching chroma key are skipped, creating transparent effect

3. **Dual blit strategy**:
   - Immediately after `graph->render()` in data updates (minimize gap)
   - Every frame in `V058DemoApp::render()` (prevent disappearance from live indicator overlaps)

**Code Pattern:**
```cpp
// V05DemoApp - Non-breaking addition
void blitTitle() {
    if (!m_titleBufferValid) renderTitleToBuffer();
    hal_display_fast_blit_transparent(x, y, w, h, m_titleBuffer, 0x0001);
}

// V058DemoApp - Data update
graph->render();         // Full-screen DMA
v05Demo->blitTitle();    // Fast DMA (1-2ms)

// V058DemoApp - Every frame
v05Demo->blitTitle();    // Restore if overwritten by live indicator
```

### Known Limitation: Slight Flash Still Visible
Despite optimization, a **minor flash is still perceptible** during graph updates due to fundamental architectural constraint:
- Graph render and title blit are **two separate DMA operations**
- Gap between operations (~1-2ms) is small but visible on high-refresh displays
- True atomic rendering would require compositing title into graph buffer before single DMA blit

**Accepted Trade-off:** Slight flash is tolerable for v0.58. Future solution would require deeper integration:
```cpp
// Hypothetical future approach
graph->setOverlay(titleBuffer, x, y, w, h);  // Composite before blit
graph->render();  // Single DMA with title included
```

### Key Lessons
1. **hal_display_flush() behavior varies by hardware** - Always check HAL implementation, never assume buffering exists
2. **Arduino_Canvas parent parameter** - Use `nullptr` for standalone canvases to avoid hardware reinitialization
3. **Chroma key transparency** - Use `0x0001` matching graph's approach for consistent transparency
4. **Buffer caching eliminates font rendering overhead** - One-time render + fast blit beats per-frame font rendering
5. **Architectural separation has cost** - Graph and overlay as separate components requires two DMA operations, creating inherent gap
6. **Perfect is enemy of good** - Slight flash is acceptable if alternative requires major architectural changes

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

The initial demo implementation (main.cpp) used approach #1: calling `g_graph->render()` every frame before drawing the indicator. This causes visible flashing because:
- The graph composite (background + data layers) is re-blitted to the display every frame
- On ESP32-S3 over SPI, this full-screen blit takes ~15-20ms
- The display refresh happens during the blit, causing tearing/flashing

### Attempted Approaches

**Attempt 1: Draw Without Erasing (FAILED)**
- Call `indicator.draw()` each frame without clearing
- **Result:** Old indicator pixels remain on screen, creating a "smear" effect
- Only the inner magenta area appeared to pulse; outer cyan edge stayed fixed

**Attempt 2: Full Graph Re-render (CURRENT - FLICKERS)**
- Call `g_graph->render()` + `indicator.draw()` each frame
- **Result:** Functionally correct but causes visible flashing artifacts
- Demonstrates the feature but not production-ready

### Correct Implementation Approaches

There are three valid solutions, each with different trade-offs:

#### Option A: Dirty-Rect in LiveIndicator (Best for Reusability)
Modify `LiveIndicator` to handle its own dirty-rect logic:
```cpp
class LiveIndicator {
    void draw(x, y) {
        // 1. Calculate union of old and new bounding boxes
        // 2. Request background restoration for old rect
        // 3. Draw new indicator
        // 4. Store new position/radius for next frame
    }
private:
    float last_x_, last_y_, last_radius_;
    bool has_drawn_once_;
};
```
**Pros:** Makes LiveIndicator truly standalone and reusable
**Cons:** Requires access to the background composite buffer or a callback mechanism

#### Option B: Application-Level Dirty Rect (Best for Demo Simplicity)
Keep the region tracking in main.cpp:
```cpp
void loop() {
    // Calculate indicator position
    // If first frame: just draw
    // Else:
    //   1. Blit old rect from graph composite
    //   2. Draw new indicator
    //   3. Store new rect
}
```
**Pros:** Keeps LiveIndicator simple, caller has full control
**Cons:** Every use case must implement this logic

#### Option C: Use Integrated TimeSeriesGraph Indicator (IMPLEMENTED)
TimeSeriesGraph already has a highly optimized indicator with dirty-rect:
- Maintains a composite buffer (bg + data)
- Tracks previous indicator position/radius
- Calculates bounding box union
- Restores background from composite
- Draws new indicator in one atomic blit

**Pros:** Already implemented and flicker-free, proven in production
**Cons:** Couples the indicator to the graph, not a standalone component

### Decision for v0.5 Demo (UPDATED)
Initially attempted **Approach B** with standalone LiveIndicator, but flashing was unacceptable for HIL testing.

**Final Implementation:** Uses **Option C (Integrated TimeSeriesGraph Indicator)**
- Flicker-free 30fps animation
- Dirty-rect optimization with composite buffer restoration
- Production-ready implementation
- Still demonstrates all v0.5 visual requirements

The standalone `LiveIndicator` component remains:
- Fully implemented and unit-tested (5 tests passing)
- Available for use cases where dirty-rect can be managed by the caller
- Documented as a reusable component for future work

**Conclusion:** For SPI-based displays where bandwidth is limited, integrated components with tight coupling to the rendering pipeline are the pragmatic choice over pure component separation.

### Key Lessons
1. **Dirty-rect optimization is not optional** for smooth animation on SPI displays
2. **Component reusability vs performance** is a real trade-off - sometimes tight coupling is the right choice
3. **Demonstrate correctness first, optimize second** - the v0.5 demo proves the concept works
4. **TimeSeriesGraph's integrated indicator** should be the reference implementation for any future dirty-rect work

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

### Root Cause
Drawing vector graphics directly to the display without dirty-rect optimization causes the same issues as the standalone LiveIndicator:
- Each frame clears background and redraws logo
- ESP32-S3 over SPI cannot complete full redraw fast enough
- Display refresh happens during draw, causing tearing/ghosting

### Solution: Dirty-Rect with Composite Buffer
Applied the same technique used in TimeSeriesGraph's live indicator:

1. **Background Composite Buffer (PSRAM):** Allocated full-screen buffer storing clean background
2. **Dirty Rect Calculation:** Calculate union of old and new logo bounding boxes
3. **Three-Step Atomic Update:**
   - Copy clean background from composite to temp buffer (erases old logo)
   - Rasterize new logo into temp buffer using barycentric coordinates
   - Single atomic blit to display via `hal_display_fast_blit`

### Implementation Details
- Logo triangles rasterized in software (barycentric test) instead of using Arduino_GFX::fillTriangle
- Allows rendering to temp buffer before display blit
- Tracks `m_lastLogoX/Y/Width/Height` for dirty-rect calculation
- Aspect ratio preserved: `widthPercent = heightPercent * (original_width / original_height)`

### Updated Animation Parameters (2026-02-06)
Re-implemented LogoScreen with corrected animation parameters per feature spec:
- **End size**: 10% of screen height
- **End anchor point**: Top-right corner of the logo image (1.0, 0.0)
- **End position**: The anchor point (top-right of logo) is placed 10px from screen edges
  - X position: `100% - (10px / screenWidth * 100%)` (10px from right edge)
  - Y position: `0% + (10px / screenHeight * 100%)` (10px from top edge)
- **Visual result**: The top-left corner of the logo ends up 10px down and to the left of the screen's top-right corner
- **Dynamic calculation**: Position calculated in `begin()` based on actual screen dimensions

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

### Implementation Details
1. Created three theme factory functions: `createGradientTheme()`, `createSolidTheme()`, `createMixedTheme()`
2. Added mode-switching timer in `loop()` that triggers every 8 seconds
3. When switching, the demo calls `setTheme()`, then redraws the background and data layers to apply the new theme
4. The live indicator continues animating smoothly during transitions using dirty-rect optimization

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
4.  **Styling**: Updated the generator to use large (24px bold) titles for category subgraphs to improve high-level architectural visibility.

### Key Lessons
1.  **Architecture is a living document**: Don't be afraid to refactor the HAL *before* it gets too messy. Segmenting by domain early prevents "Header Spaghetti."
2.  **Browser > Terminal for Visualization**: Browsers offer superior interactive capabilities (zoom, search, pan) for complex diagrams.
3.  **Gherkin Prerequisite Formatting**: Discovered that the dependency parser is sensitive to formatting. Multiple prerequisites MUST be on separate lines (`> Prerequisite: X \n > Prerequisite: Y`) to be parsed correctly by the regex.

### Problem
The demo_release_0.5.md spec requires two layout modes: "Scientific" (OUTSIDE tick labels + axis titles) and "Compact" (INSIDE tick labels, no titles). Previous attempts to modify the graph draw methods caused watchdog reset (TG1WDT_SYS_RST) crashes.

### Root Cause of Previous Crashes
Including `theme_manager.h` directly in `ui_time_series_graph.cpp` pulled in all 5 font data arrays (~100KB+ of static data). Combined with the draw method modifications, this likely caused memory pressure or stack overflow during the pixel-intensive background rendering.

### Solution: Font Injection via GraphTheme Struct
Instead of the graph code accessing `ThemeManager::getInstance()` directly, fonts are passed through the `GraphTheme` struct:
- `tickFont`: For tick labels (9pt smallest)
- `axisTitleFont`: For axis titles (18pt UI)

The demo code (which already includes `theme_manager.h`) sets these when creating themes.

### Key Design Decisions
1. **Dynamic margins via `getMargins()`** replace static `constexpr` values. Returns wider margins for OUTSIDE mode (room for external labels + titles) and narrow margins for INSIDE mode (labels overlay plot area).
2. **Y-tick labels skip first tick** (at y_min) to avoid overlap with X-axis.
3. **X-tick labels skip first tick** (at index 0) to avoid overlap with Y-axis.
4. **Y-axis title rendered vertically** using character-by-character drawing (centered in graph area).
5. **Demo cycles 6 combinations**: 2 layout modes × 3 visual styles every 5 seconds.

### Key Lessons
1. **Never include font headers in draw code** - pass font pointers through configuration structs instead
2. **Watchdog crashes on ESP32-S3** are often caused by memory pressure during pixel-intensive operations, not necessarily by timing
3. **Incremental modification of draw methods** after full revert is safer than trying to fix crashes in-place
4. **CRITICAL: Custom GFX fonts crash on PSRAM Arduino_Canvas** - calling `canvas->setFont(GFXfont*)` on an Arduino_Canvas allocated in PSRAM causes TG1WDT_SYS_RST watchdog resets. **Workaround:** Use built-in font with `canvas->setFont(nullptr); canvas->setTextSize(N);` instead. This limitation means axis titles use 10x14 pixel characters (size 2) and tick labels use 5x7 pixel characters (size 1), instead of the 9pt and 18pt custom fonts specified in the theme.

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
**Root Cause:** Phase check `isInVisualPhase()` returned true during logo animation because V055's `PHASE_VISUAL_DEMO` includes both logo and graph stages.
**Solution:** Added granular stage checking:
- V055 Phase check: `isInVisualPhase()` (not connectivity/handover)
- V05 Stage check: `isShowingGraph()` (STAGE_GRAPH_CYCLE, not STAGE_LOGO)
- Combined: `v055Demo->isInVisualPhase() && v05Demo->isShowingGraph()`
**Result:** Graph updates only during active graph display, logo completes uninterrupted.

#### Challenge 2: Live Indicator Misalignment
**Problem:** After data updates, live indicator didn't track the last data point on the graph.
**Root Cause:** Graph data canvas was updated (`setData()` + `drawData()`) but not composited to display. V05DemoApp's render flow only calls `graph->render()` during mode switches (every 5s), not on every frame.
**Solution:** Call `graph->render()` in `updateGraphWithLiveData()` after `drawData()` to composite and blit updated data canvas.
**Result:** Live indicator now tracks rightmost data point accurately.

#### Challenge 3: Title Text Flickering (PARTIALLY RESOLVED)
**Problem:** "DEMO v0.58" title flashes when graph updates every 3 seconds.
**Root Cause:** Dual rendering paths with async synchronization:
- `graph->render()` → `hal_display_fast_blit()` → Direct DMA to display hardware (immediate)
- `drawTitle()` → Arduino_GFX framebuffer → Requires `hal_display_flush()` to send to display

**Attempted Solutions:**
1. **Initial:** Redraw title after `graph->render()`, rely on normal flush in V05DemoApp::render()
   - **Result:** Visible flicker due to timing gap between DMA blit and framebuffer flush
2. **Current:** Call `hal_display_flush()` immediately after `drawTitle()` to force sync
   - **Result:** Reduced flicker but still visible artifact

**Status:** Under investigation. The fundamental issue is that graph uses DMA (bypasses framebuffer) while title uses GFX framebuffer (async path). Potential solutions:
- Draw title to a buffer before final composite (requires graph architecture changes)
- Use double-buffering for entire display (memory intensive)
- Accept minor flicker as trade-off for live updates

### Implementation Pattern: Wrapper Composition

V058DemoApp follows the established wrapper pattern:
```
V058DemoApp (Live Data)
  └─ wraps V055DemoApp (WiFi + Connectivity)
      └─ wraps V05DemoApp (Logo + 6 Graph Modes)
```

**Data Flow:**
1. V058 creates `DataItemTimeSeries` with embedded test data
2. Timer triggers `injectNewDataPoint()` every 3 seconds
3. Random data added to FIFO buffer (oldest evicted)
4. `updateGraphWithLiveData()` checks phase/stage gates
5. If in graph display: `setData()` → `drawData()` → `render()` → `drawTitle()` → `flush()`

### Key Lessons
1. **Embedded data > Filesystem for demos** - Simpler deployment, no upload dependencies
2. **FIFO sizing must match use case** - Set max_length to match expected window size, not arbitrary buffer
3. **Fixed bounds prevent drift** - Store initial data range for random generation, don't use dynamic min/max
4. **Granular phase checking required** - Single-level phase checks insufficient when phases have sub-stages
5. **DMA vs Framebuffer sync is hard** - Direct hardware blitting (DMA) and framebuffer rendering are fundamentally async, perfect sync may not be achievable without architectural changes
6. **Title redraw frequency matters** - Redrawing on every graph update (3s) instead of mode switch (5s) increases flicker opportunity
7. **Data update interval trade-off** - Increased from 1s to 3s to reduce rendering pressure and flicker frequency