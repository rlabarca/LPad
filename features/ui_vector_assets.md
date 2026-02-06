> Prerequisite: features/display_relative_drawing.md

# Feature: Vector Asset Pipeline

> Label: "Vector Asset Pipeline"
> Category: "Graphics Engine"

## Introduction

This feature establishes a pipeline to compile SVG assets into optimized C++ data structures (vector meshes) that can be rendered by the application. This allows for resolution-independent, scalable graphics (like logos and icons) without the runtime overhead of a full SVG parser.

## 1. Asset Compilation Script

The Builder must create a Python script `scripts/process_svgs.py` that is hooked into the PlatformIO build process (or run manually via a helper script).

### Requirements:
*   **Input:** Scans `assets/*.svg`.
*   **Processing:**
    *   Parses `<path>` elements containing basic drawing commands.
    *   Specifically supports the `d` attribute with `M` (MoveTo), `L` (LineTo), and `Z` (ClosePath) commands, which form polygons.
    *   Extracts the `fill` color (hex format) and converts it to RGB565 (uint16_t).
    *   **Normalization:** Calculates the bounding box of the shape and normalizes all vertex coordinates to the range `[0.0, 1.0]`, preserving the aspect ratio.
*   **Output:** Generates `src/generated/vector_assets.h` and `src/generated/vector_assets.cpp`.
    *   Defines a namespace `VectorAssets`.
    *   Exports `extern const VectorShape NameOfAsset;` for each SVG file (e.g., `LPadLogo`).

### Data Structures (C++):
The script should generate code compatible with the following structs (to be defined in `src/vector_renderer.h`):

```cpp
struct VectorVertex {
    float x; // 0.0 to 1.0
    float y; // 0.0 to 1.0
};

struct VectorTriangle {
    VectorVertex v1;
    VectorVertex v2;
    VectorVertex v3;
};

struct VectorPath {
    uint16_t color;      // RGB565
    size_t num_tris;
    const VectorTriangle* tris;
};

struct VectorShape {
    size_t num_paths;
    const VectorPath* paths;
    float original_width;
    float original_height;
};
```

## 2. Vector Renderer

The Builder must create a rendering utility, likely `src/vector_renderer.h` (and `.cpp`), that integrates with `RelativeDisplay`.

### API:
```cpp
class VectorRenderer {
public:
    static void draw(
        RelativeDisplay& display,
        const VectorShape& shape,
        float x_percent,      // Target X position (0-100)
        float y_percent,      // Target Y position (0-100)
        float scale_x,        // Scale factor for width (1.0 = original normalized size relative to screen width?) 
                              // *Clarification*: Scale should probably be relative to screen dimensions.
                              // Let's say: scale=1.0 means the shape's 1.0 normalized dimension equals 100% of screen width/height? 
                              // No, usually we want "width in percent".
        float width_percent,  // The desired width of the shape in % of screen width. Height is calculated to maintain aspect ratio.
        float anchor_x,       // Anchor point within the shape (0.0=left, 0.5=center, 1.0=right)
        float anchor_y        // Anchor point within the shape (0.0=top, 0.5=center, 1.0=bottom)
    );
};
```

### Rendering Logic:
1.  Iterate over all paths in the `VectorShape`.
2.  For each triangle in a path:
    *   Transform each vertex `v` (which is 0..1):
        *   `v_adj.x = v.x - anchor_x`
        *   `v_adj.y = v.y - anchor_y`
        *   `screen_x = base_x + v_adj.x * target_width`
        *   `screen_y = base_y + v_adj.y * target_height` (where target_height is derived from aspect ratio).
    *   Draw the triangle using `display.getGfx()->fillTriangle(...)`.

## 3. Scenarios

### Scenario: Compile Assets
- **Given** an SVG file `assets/LPadLogo.svg` exists.
- **When** the `scripts/process_svgs.py` script is executed.
- **Then** `src/generated/vector_assets.h` should contain `extern const VectorShape LPadLogo;`.
- **And** `src/generated/vector_assets.cpp` should contain the triangle data for the logo.

### Scenario: Draw Scaled Asset
- **Given** `VectorAssets::LPadLogo` is available.
- **When** `VectorRenderer::draw(display, VectorAssets::LPadLogo, 50.0f, 50.0f, 20.0f, ...)` is called (Center at 50,50, width 20%).
- **Then** the logo should be drawn centered on the screen, occupying 20% of the screen width.
