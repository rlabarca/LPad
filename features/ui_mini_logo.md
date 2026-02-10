# Mini Logo UI Component

> Label: "UI Mini Logo"
> Category: "UI Framework"
> Prerequisite: features/ui_logo_screen.md

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
