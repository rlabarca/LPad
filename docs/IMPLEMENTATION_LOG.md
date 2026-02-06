# Implementation Log & Lessons Learned

This document captures the "tribal knowledge" of the project: technical hurdles, why specific decisions were made, and what approaches were discarded.

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

## [2026-02-05] Tick Label Positioning (INSIDE/OUTSIDE) and Axis Titles

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