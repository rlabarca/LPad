#include "vector_renderer.h"
#include "../hal/display.h"
#include <Arduino_GFX_Library.h>

// Transparent color for canvas-based rendering (bright magenta — unlikely in real content)
static const uint16_t VECTOR_TRANSPARENT = 0xF81F;

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
    int32_t screen_width = hal_display_get_width_pixels();
    int32_t screen_height = hal_display_get_height_pixels();
    float screen_aspect_ratio = static_cast<float>(screen_width) / static_cast<float>(screen_height);

    // Calculate target dimensions in percent
    float target_width = width_percent;
    float target_height = width_percent * shape_aspect_ratio * screen_aspect_ratio;

    // Calculate base position adjusted for anchor
    float base_x = x_percent - (anchor_x * target_width);
    float base_y = y_percent - (anchor_y * target_height);

    // First pass: compute pixel bounding box of all triangles
    int32_t min_px = screen_width, min_py = screen_height;
    int32_t max_px = 0, max_py = 0;

    for (size_t pi = 0; pi < shape.num_paths; pi++) {
        const VectorPath& path = shape.paths[pi];
        for (size_t ti = 0; ti < path.num_tris; ti++) {
            const VectorTriangle& tri = path.tris[ti];
            int32_t vx[3], vy[3];
            vx[0] = display.relativeToAbsoluteX(base_x + tri.v1.x * target_width);
            vy[0] = display.relativeToAbsoluteY(base_y + tri.v1.y * target_height);
            vx[1] = display.relativeToAbsoluteX(base_x + tri.v2.x * target_width);
            vy[1] = display.relativeToAbsoluteY(base_y + tri.v2.y * target_height);
            vx[2] = display.relativeToAbsoluteX(base_x + tri.v3.x * target_width);
            vy[2] = display.relativeToAbsoluteY(base_y + tri.v3.y * target_height);
            for (int v = 0; v < 3; v++) {
                if (vx[v] < min_px) min_px = vx[v];
                if (vy[v] < min_py) min_py = vy[v];
                if (vx[v] > max_px) max_px = vx[v];
                if (vy[v] > max_py) max_py = vy[v];
            }
        }
    }

    // Clamp to screen bounds
    if (min_px < 0) min_px = 0;
    if (min_py < 0) min_py = 0;
    if (max_px >= screen_width) max_px = screen_width - 1;
    if (max_py >= screen_height) max_py = screen_height - 1;

    int16_t cw = static_cast<int16_t>(max_px - min_px + 1);
    int16_t ch = static_cast<int16_t>(max_py - min_py + 1);
    if (cw <= 0 || ch <= 0) return;

    // Try to create a temporary canvas for shadow-buffer-aware rendering
    hal_canvas_handle_t canvas = hal_display_canvas_create(cw, ch);
    Arduino_GFX* draw_target;
    int32_t offset_x, offset_y;
    bool using_canvas = false;

    if (canvas) {
        hal_display_canvas_fill(canvas, VECTOR_TRANSPARENT);
        draw_target = static_cast<Arduino_GFX*>(
            static_cast<Arduino_Canvas*>(canvas));
        offset_x = min_px;
        offset_y = min_py;
        using_canvas = true;
    } else {
        // Fallback: draw directly to GFX (no shadow buffer capture)
        draw_target = display.getGfx();
        offset_x = 0;
        offset_y = 0;
    }

    // Second pass: draw all triangles
    for (size_t pi = 0; pi < shape.num_paths; pi++) {
        const VectorPath& path = shape.paths[pi];
        for (size_t ti = 0; ti < path.num_tris; ti++) {
            const VectorTriangle& tri = path.tris[ti];

            int32_t x1 = display.relativeToAbsoluteX(base_x + tri.v1.x * target_width) - offset_x;
            int32_t y1 = display.relativeToAbsoluteY(base_y + tri.v1.y * target_height) - offset_y;
            int32_t x2 = display.relativeToAbsoluteX(base_x + tri.v2.x * target_width) - offset_x;
            int32_t y2 = display.relativeToAbsoluteY(base_y + tri.v2.y * target_height) - offset_y;
            int32_t x3 = display.relativeToAbsoluteX(base_x + tri.v3.x * target_width) - offset_x;
            int32_t y3 = display.relativeToAbsoluteY(base_y + tri.v3.y * target_height) - offset_y;

            draw_target->fillTriangle(x1, y1, x2, y2, x3, y3, path.color);
        }
    }

    // Blit canvas to display via HAL (updates both display and shadow buffer)
    if (using_canvas) {
        Arduino_Canvas* canvas_ptr = static_cast<Arduino_Canvas*>(canvas);
        uint16_t* fb = canvas_ptr->getFramebuffer();
        if (fb) {
            hal_display_fast_blit_transparent(
                static_cast<int16_t>(min_px), static_cast<int16_t>(min_py),
                cw, ch, fb, VECTOR_TRANSPARENT);
        }
        hal_display_canvas_delete(canvas);
    }
}
