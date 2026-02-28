#pragma once

#include "relative_display.h"

/**
 * MiniLogo - Renders a small, static LPad logo in a specified corner of the screen.
 *
 * This component provides a simple way to display the LPad logo at a fixed small size
 * in any corner of the display. It uses the existing vector rendering infrastructure.
 */
class MiniLogo {
public:
    /**
     * Corner position for the logo
     */
    enum class Corner {
        TOP_LEFT,
        TOP_RIGHT,
        BOTTOM_LEFT,
        BOTTOM_RIGHT
    };

    /**
     * Constructor
     * @param display RelativeDisplay instance to render to
     * @param corner Corner position for the logo
     */
    MiniLogo(RelativeDisplay* display, Corner corner);

    /**
     * Render the mini logo to the display
     * Note: This does not call hal_display_flush(). The caller is responsible
     * for flushing the display after all drawing operations are complete.
     */
    void render();

    /**
     * Change the logo's corner position
     * @param corner New corner position
     */
    void setCorner(Corner corner);

    /**
     * Get the current corner position
     * @return Current corner
     */
    Corner getCorner() const { return m_corner; }

private:
    RelativeDisplay* m_display;
    Corner m_corner;

    // Logo size as percentage of screen height (matches LogoScreen end size)
    static constexpr float LOGO_HEIGHT_PERCENT = 10.0f;

    // Offset from corner edges in pixels
    static constexpr float CORNER_OFFSET_PX = 10.0f;

    // Calculate position and anchor based on corner
    void calculatePositionAndAnchor(float& out_x, float& out_y, float& out_anchor_x, float& out_anchor_y);
};
