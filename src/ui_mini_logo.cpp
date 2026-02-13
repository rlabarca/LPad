#include "ui_mini_logo.h"
#include "vector_renderer.h"
#include "generated/vector_assets.h"

MiniLogo::MiniLogo(RelativeDisplay* display, Corner corner)
    : m_display(display)
    , m_corner(corner)
{
}

void MiniLogo::render() {
    if (m_display == nullptr) return;

    // Calculate position and anchor based on corner
    float x_percent, y_percent, anchor_x, anchor_y;
    calculatePositionAndAnchor(x_percent, y_percent, anchor_x, anchor_y);

    // Calculate width maintaining aspect ratio
    // The logo is 245x370 (W x H), which is portrait (taller than wide)
    float logoAspectRatio = VectorAssets::Lpadlogo.original_width /
                           VectorAssets::Lpadlogo.original_height;  // 245/370 = 0.662

    int32_t screen_width = m_display->getWidth();
    int32_t screen_height = m_display->getHeight();
    float screenAspectRatio = static_cast<float>(screen_height) / static_cast<float>(screen_width);

    // Width percent needed to maintain logo aspect ratio at given height percent
    float widthPercent = LOGO_HEIGHT_PERCENT * screenAspectRatio * logoAspectRatio;

    // Render the logo using VectorRenderer
    VectorRenderer::draw(*m_display, VectorAssets::Lpadlogo,
                        x_percent, y_percent, widthPercent,
                        anchor_x, anchor_y);
}

void MiniLogo::setCorner(Corner corner) {
    m_corner = corner;
}

void MiniLogo::calculatePositionAndAnchor(float& out_x, float& out_y,
                                         float& out_anchor_x, float& out_anchor_y) {
    // Get screen dimensions from RelativeDisplay
    int32_t screen_width = m_display->getWidth();
    int32_t screen_height = m_display->getHeight();

    // Convert pixel offset to percentage
    float offsetX_percent = (CORNER_OFFSET_PX / static_cast<float>(screen_width)) * 100.0f;
    float offsetY_percent = (CORNER_OFFSET_PX / static_cast<float>(screen_height)) * 100.0f;

    // Calculate position and anchor based on corner
    // RelativeDisplay: X=0 is left, X=100 is right
    // RelativeDisplay: Y=0 is top, Y=100 is bottom
    switch (m_corner) {
        case Corner::TOP_LEFT:
            out_x = offsetX_percent;        // Left edge + offset
            out_y = offsetY_percent;        // Top edge + offset
            out_anchor_x = 0.0f;            // Left anchor
            out_anchor_y = 0.0f;            // Top anchor
            break;

        case Corner::TOP_RIGHT:
            out_x = 100.0f - offsetX_percent;  // Right edge - offset
            out_y = offsetY_percent;           // Top edge + offset
            out_anchor_x = 1.0f;               // Right anchor
            out_anchor_y = 0.0f;               // Top anchor
            break;

        case Corner::BOTTOM_LEFT:
            out_x = offsetX_percent;           // Left edge + offset
            out_y = 100.0f - offsetY_percent;  // Bottom edge - offset
            out_anchor_x = 0.0f;               // Left anchor
            out_anchor_y = 1.0f;               // Bottom anchor
            break;

        case Corner::BOTTOM_RIGHT:
            out_x = 100.0f - offsetX_percent;  // Right edge - offset
            out_y = 100.0f - offsetY_percent;  // Bottom edge - offset
            out_anchor_x = 1.0f;               // Right anchor
            out_anchor_y = 1.0f;               // Bottom anchor
            break;
    }
}
