# Feature: Vector Renderer

> Label: "Render: Vector"
> Category: "Rendering"
> Prerequisite: features/arch_display_pipeline.md

[TODO]

## 1. Overview

The VectorRenderer is a static utility that renders triangulated SVG assets (VectorShape data) onto a RelativeDisplay using relative coordinates with configurable anchor points. It preserves aspect ratio by computing target height from the shape's original aspect ratio corrected for screen aspect. Rendering uses a temporary PSRAM canvas with transparency for clean compositing, with a direct-draw fallback if canvas allocation fails.

---

## 2. Requirements

### 2.1 Coordinate Mapping

- Target position specified as percentage of screen (0-100%).
- Anchor point (0.0-1.0 on each axis) determines the pivot: 0.5/0.5 = center, 1.0/0.0 = top-right.
- Target height auto-calculated from width to preserve the VectorShape's original aspect ratio, corrected for screen aspect ratio.

### 2.2 Rendering Pipeline

- Pass 1: compute pixel bounding box of all triangles across all paths. Clamp to screen bounds.
- Allocate temporary PSRAM canvas at the bounding box size, filled with chroma key (0x0001).
- Pass 2: draw all `fillTriangle()` calls to the canvas using per-path RGB565 colors, offset by bounding box origin.
- Blit canvas to display via `hal_display_fast_blit_transparent()` (skips chroma key pixels).
- Fallback: if canvas creation fails, draw triangles directly to the display GFX (no transparency).

### 2.3 Input Data

- `VectorShape` contains an array of `VectorPath` entries, each with an RGB565 color and an array of `VectorTriangle` (3 vertices in normalized 0-1 coordinates).
- `original_width` and `original_height` from the SVG viewBox are stored for aspect ratio calculation.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Aspect ratio is preserved

    Given a VectorShape with original dimensions 245x370
    And target width is 50% of a 400x400 screen
    When draw is called
    Then the rendered height preserves the 245:370 aspect ratio

#### Scenario: Anchor point offsets correctly

    Given anchor (0.5, 0.5) and position (50%, 50%)
    When draw is called
    Then the shape is centered on screen

#### Scenario: Canvas fallback on OOM

    Given PSRAM canvas creation fails
    When draw is called
    Then triangles are drawn directly to the display GFX
    And no crash occurs

#### Scenario: Chroma key transparency works

    Given a VectorShape with sparse triangles
    When rendered via canvas and blitted
    Then chroma key pixels (0x0001) are not drawn to the display

### Manual Scenarios (Human Verification Required)

#### Scenario: Vector logo renders cleanly on device

    Given the LPad logo SVG asset is loaded
    When VectorRenderer draws it at 75% screen height
    Then the logo appears with correct colors and no visible artifacts
