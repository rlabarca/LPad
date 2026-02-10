#include "vector_renderer.h"
#include "../hal/display.h"
#include <Arduino_GFX_Library.h>

void VectorRenderer::draw(
    RelativeDisplay& display,
    const VectorShape& shape,
    float x_percent,
    float y_percent,
    float width_percent,
    float anchor_x,
    float anchor_y
) {
    // Calculate aspect ratio from original dimensions
    float shape_aspect_ratio = shape.original_height / shape.original_width;

    // Get screen dimensions to account for screen aspect ratio
    // (VectorRenderer works in percentage coordinates where width% and height%
    // are relative to screen width and height respectively, so we must convert)
    int32_t screen_width = hal_display_get_width_pixels();
    int32_t screen_height = hal_display_get_height_pixels();
    float screen_aspect_ratio = static_cast<float>(screen_width) / static_cast<float>(screen_height);

    // Calculate target dimensions in percent
    // width_percent is % of screen width
    // target_height must be % of screen height (accounting for both shape and screen aspect ratios)
    float target_width = width_percent;
    float target_height = width_percent * shape_aspect_ratio * screen_aspect_ratio;

    // Calculate base position adjusted for anchor
    float base_x = x_percent - (anchor_x * target_width);
    float base_y = y_percent - (anchor_y * target_height);

    // Get underlying GFX object for direct triangle drawing
    Arduino_GFX* gfx = display.getGfx();

    // Render all paths
    for (size_t path_idx = 0; path_idx < shape.num_paths; path_idx++) {
        const VectorPath& path = shape.paths[path_idx];

        // Render all triangles in this path
        for (size_t tri_idx = 0; tri_idx < path.num_tris; tri_idx++) {
            const VectorTriangle& tri = path.tris[tri_idx];

            // Transform vertices from normalized [0,1] to screen percent
            // Adjust for anchor point first
            float v1_x = base_x + (tri.v1.x * target_width);
            float v1_y = base_y + (tri.v1.y * target_height);

            float v2_x = base_x + (tri.v2.x * target_width);
            float v2_y = base_y + (tri.v2.y * target_height);

            float v3_x = base_x + (tri.v3.x * target_width);
            float v3_y = base_y + (tri.v3.y * target_height);

            // Convert to absolute pixel coordinates
            int32_t x1 = display.relativeToAbsoluteX(v1_x);
            int32_t y1 = display.relativeToAbsoluteY(v1_y);

            int32_t x2 = display.relativeToAbsoluteX(v2_x);
            int32_t y2 = display.relativeToAbsoluteY(v2_y);

            int32_t x3 = display.relativeToAbsoluteX(v3_x);
            int32_t y3 = display.relativeToAbsoluteY(v3_y);

            // Draw the filled triangle
            gfx->fillTriangle(x1, y1, x2, y2, x3, y3, path.color);
        }
    }
}
