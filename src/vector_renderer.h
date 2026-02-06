#pragma once

#include "relative_display.h"
#include "generated/vector_assets.h"

/**
 * VectorRenderer - Renders vector shapes (triangulated SVG assets) to a RelativeDisplay.
 *
 * Transforms normalized [0,1] vertex coordinates to screen space based on positioning,
 * scaling, and anchor point parameters.
 */
class VectorRenderer {
public:
    /**
     * Draw a vector shape to the display.
     *
     * @param display RelativeDisplay instance to draw to
     * @param shape VectorShape data (from generated/vector_assets.h)
     * @param x_percent Target X position in percent (0-100)
     * @param y_percent Target Y position in percent (0-100)
     * @param width_percent Desired width of the shape in percent of screen width
     * @param anchor_x Anchor point within the shape X (0.0=left, 0.5=center, 1.0=right)
     * @param anchor_y Anchor point within the shape Y (0.0=top, 0.5=center, 1.0=bottom)
     */
    static void draw(
        RelativeDisplay& display,
        const VectorShape& shape,
        float x_percent,
        float y_percent,
        float width_percent,
        float anchor_x = 0.5f,
        float anchor_y = 0.5f
    );
};
