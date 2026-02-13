# Mini Logo UI Component

> Label: "UI Mini Logo"
> Category: "UI Framework"
> Prerequisite: features/ui_vector_assets.md

This feature defines a small, static logo component that can be drawn in any corner of the screen.

## Scenarios

### Scenario: Draw Mini Logo

- **Given** a `RelativeDisplay` instance
- **And** a `MiniLogo` instance initialized for the top-right corner
- **When** `render()` is called on the `MiniLogo` instance
- **Then** the LPad logo should be drawn at a small, fixed size in the top-right corner of the display.
- **And** the background behind the logo should be transparent.

### Scenario: Change Logo Position

- **Given** a `MiniLogo` instance initialized for the top-right corner
- **When** the logo's position is changed to the bottom-left corner
- **And** `render()` is called
- **Then** the logo should be drawn in the bottom-left corner.

## Implementation Details

The `MiniLogo` class should:
- Take a `RelativeDisplay` pointer and a corner enum (e.g., `TOP_LEFT`, `TOP_RIGHT`, `BOTTOM_LEFT`, `BOTTOM_RIGHT`) in its constructor.
- Have a `render()` method that draws the logo.
- Use the existing vector assets for the logo (`LPadLogo_Path_Data`, `LPadLogo_Path_Count`).
- The logo should be rendered at a fixed small size, regardless of screen resolution. A good starting point would be around 10-15% of the screen's smaller dimension.
- The rendering should be efficient and not cause significant performance overhead, especially when drawn on top of other frequently updated UI elements like a graph. The implementation should consider if pre-rendering the logo to a buffer is necessary.

## HAL Dependencies

- `hal_display` for drawing operations.
- `relative_display` for coordinate calculations.

## Test Plan

- **Unit Test:**
  - Verify that the `MiniLogo` class can be instantiated for each corner.
  - Mock the `RelativeDisplay` and verify that the `render()` method calls the correct drawing functions with the expected coordinates and size for each corner.
- **HIL Test:**
  - Create a simple demo that draws the mini logo in each of the four corners, pausing for a few seconds at each position.
  - This will be part of the main v0.60 demo.

## Implementation Notes

### [2026-02-09] VectorRenderer Aspect Ratio Bug
**Problem:** Mini logo appeared vertically squashed — VectorRenderer calculated `target_height = width_percent * shape_aspect_ratio` without accounting for screen aspect ratio.
**Fix:** `target_height = width_percent * shape_aspect_ratio * (screen_width / screen_height)`. Percentage-based coordinates must account for screen dimensions when converting between width% and height%.

### [2026-02-09] Size Consistency Between LogoScreen and MiniLogo
**Problem:** MiniLogo header defined 12% height vs LogoScreen's 10% end size, causing a visible jump during transition.
**Fix:** Changed MiniLogo `LOGO_HEIGHT_PERCENT` from 12.0f to 10.0f to match LogoScreen end size. When one component transitions to another, sizes must match exactly.

### [2026-02-06] LogoScreen Dirty-Rect Optimization
**Problem:** Drawing logo directly to display each frame caused visible refresh artifacts during smooth EaseInOut animation.
**Solution:** Same "Layered Rendering" approach as TimeSeriesGraph:
1. Background Composite Buffer (PSRAM) stores clean background
2. Dirty Rect = union of old and new logo bounding boxes
3. Three-step atomic update: restore clean BG → rasterize new logo → single `hal_display_fast_blit`

### [2026-02-06] LogoScreen Animation Parameters
- **End size:** 10% of screen height
- **End anchor point:** Top-right corner of the logo image (1.0, 0.0)
- **End position:** Anchor point placed 10px from screen edges
- **Critical:** Anchor (1.0, 0.0) = top-right of LOGO IMAGE, not screen. Logo extends leftward and downward from that point.
