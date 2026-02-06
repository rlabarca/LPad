#include "ui_logo_screen.h"
#include "vector_renderer.h"
#include "display.h"
#include <algorithm>
#include <cstring>

// Start state from spec (centered, large, center anchor)
static constexpr float START_POS_X = 50.0f;
static constexpr float START_POS_Y = 50.0f;
static constexpr float START_HEIGHT = 75.0f;  // 75% of screen height
static constexpr float START_ANCHOR_X = 0.5f;
static constexpr float START_ANCHOR_Y = 0.5f;

// End state from spec (top-right, small, top-left anchor)
// Position offset by 10px from top-right corner
static constexpr float END_HEIGHT = 10.0f;  // 10% of screen height
static constexpr float END_ANCHOR_X = 0.0f;  // Top-left anchor
static constexpr float END_ANCHOR_Y = 0.0f;
static constexpr float CORNER_OFFSET_PX = 10.0f;  // Offset from corner in pixels

LogoScreen::LogoScreen(float waitDuration, float animDuration)
    : m_waitDuration(waitDuration)
    , m_animDuration(animDuration)
    , m_timer(0.0f)
    , m_state(State::WAIT)
    , m_display(nullptr)
    , m_gfx(nullptr)
    , m_width(0)
    , m_height(0)
    , m_backgroundColor(0x0000)
    , m_compositeBuffer(nullptr)
    , m_hasDrawnLogo(false)
    , m_lastLogoX(0)
    , m_lastLogoY(0)
    , m_lastLogoWidth(0)
    , m_lastLogoHeight(0)
{
    // Initialize start state (will be copied to m_current in begin())
    m_startParams.posX = START_POS_X;
    m_startParams.posY = START_POS_Y;
    m_startParams.heightPercent = START_HEIGHT;
    m_startParams.anchorX = START_ANCHOR_X;
    m_startParams.anchorY = START_ANCHOR_Y;

    // End state will be calculated in begin() based on screen dimensions
    m_endParams.heightPercent = END_HEIGHT;
    m_endParams.anchorX = END_ANCHOR_X;
    m_endParams.anchorY = END_ANCHOR_Y;
    m_endParams.posX = 0.0f;  // Calculated in begin()
    m_endParams.posY = 0.0f;  // Calculated in begin()

    // Set initial position
    m_current = m_startParams;
}

LogoScreen::~LogoScreen() {
    if (m_compositeBuffer != nullptr) {
        free(m_compositeBuffer);
        m_compositeBuffer = nullptr;
    }
}

bool LogoScreen::begin(RelativeDisplay* display, uint16_t backgroundColor) {
    if (display == nullptr) return false;

    m_display = display;
    m_gfx = display->getGfx();
    m_width = hal_display_get_width_pixels();
    m_height = hal_display_get_height_pixels();
    m_backgroundColor = backgroundColor;

    // Calculate end position: top-left corner at (ScreenWidth - 10, 10) in screen pixels
    // In RelativeDisplay coords (where Y=0 is bottom, Y=100 is top):
    // - X = (ScreenWidth - 10) / ScreenWidth * 100
    // - Y = (ScreenHeight - 10) / ScreenHeight * 100
    float offsetX_percent = (CORNER_OFFSET_PX / static_cast<float>(m_width)) * 100.0f;
    float offsetY_percent = (CORNER_OFFSET_PX / static_cast<float>(m_height)) * 100.0f;

    m_endParams.posX = 100.0f - offsetX_percent;  // Right edge minus offset
    m_endParams.posY = 100.0f - offsetY_percent;  // Top edge minus offset

    // Allocate composite buffer (background) in PSRAM if available
    size_t buffer_size = static_cast<size_t>(m_width) * static_cast<size_t>(m_height);
    m_compositeBuffer = static_cast<uint16_t*>(
#ifdef BOARD_HAS_PSRAM
        ps_malloc(buffer_size * sizeof(uint16_t))
#else
        malloc(buffer_size * sizeof(uint16_t))
#endif
    );

    if (m_compositeBuffer == nullptr) {
        return false;
    }

    // Draw full screen background to display and capture to composite buffer
    m_display->drawSolidBackground(backgroundColor);
    hal_display_flush();

    // Copy the displayed background to composite buffer
    // (In a real implementation, we'd capture directly from the display buffer,
    // but for now we just fill with the background color)
    for (size_t i = 0; i < buffer_size; i++) {
        m_compositeBuffer[i] = backgroundColor;
    }

    // Reset state
    m_state = State::WAIT;
    m_timer = 0.0f;
    m_hasDrawnLogo = false;

    // Draw initial frame
    renderLogo();

    return true;
}

LogoScreen::State LogoScreen::update(float deltaTime) {
    if (m_state == State::DONE || m_display == nullptr) {
        return m_state;
    }

    m_timer += deltaTime;

    switch (m_state) {
        case State::WAIT:
            if (m_timer >= m_waitDuration) {
                // Transition to animation phase
                m_state = State::ANIMATE;
                m_timer = 0.0f;
            }
            break;

        case State::ANIMATE:
            if (m_timer >= m_animDuration) {
                // Animation complete
                m_state = State::DONE;
                m_timer = m_animDuration;
                updateAnimParams(1.0f);  // Ensure final position
            } else {
                // Update animation
                float t = m_timer / m_animDuration;
                updateAnimParams(easeInOutCubic(t));
            }
            // Render new frame during animation
            renderLogo();
            break;

        case State::DONE:
            break;
    }

    return m_state;
}

float LogoScreen::easeInOutCubic(float t) {
    // Clamp t to [0, 1]
    t = std::max(0.0f, std::min(1.0f, t));

    // EaseInOutCubic formula
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float f = (2.0f * t - 2.0f);
        return 0.5f * f * f * f + 1.0f;
    }
}

void LogoScreen::updateAnimParams(float t) {
    // Linear interpolation of all parameters from start to end
    m_current.posX = m_startParams.posX + (m_endParams.posX - m_startParams.posX) * t;
    m_current.posY = m_startParams.posY + (m_endParams.posY - m_startParams.posY) * t;
    m_current.heightPercent = m_startParams.heightPercent + (m_endParams.heightPercent - m_startParams.heightPercent) * t;
    m_current.anchorX = m_startParams.anchorX + (m_endParams.anchorX - m_startParams.anchorX) * t;
    m_current.anchorY = m_startParams.anchorY + (m_endParams.anchorY - m_startParams.anchorY) * t;
}

void LogoScreen::reset() {
    // Reset to initial state
    m_state = State::WAIT;
    m_timer = 0.0f;
    m_hasDrawnLogo = false;

    // Reset to start position
    m_current = m_startParams;

    // Clear screen and redraw background
    if (m_display != nullptr && m_compositeBuffer != nullptr) {
        m_display->drawSolidBackground(m_backgroundColor);
        hal_display_flush();

        // Refill composite buffer
        size_t buffer_size = static_cast<size_t>(m_width) * static_cast<size_t>(m_height);
        for (size_t i = 0; i < buffer_size; i++) {
            m_compositeBuffer[i] = m_backgroundColor;
        }

        // Draw initial frame
        renderLogo();
    }
}

void LogoScreen::calculateBoundingBox(const AnimParams& params,
                                     int32_t& out_x, int32_t& out_y,
                                     int32_t& out_width, int32_t& out_height) {
    // Calculate logo dimensions preserving aspect ratio
    // The logo is 245x370 (W x H), which is portrait (taller than wide)
    // We want heightPercent of screen height, so calculate width accordingly

    float logoAspectRatio = VectorAssets::Lpadlogo.original_width /
                           VectorAssets::Lpadlogo.original_height;  // 245/370 = 0.662

    // Account for screen aspect ratio when converting height% to width%
    float screenAspectRatio = static_cast<float>(m_height) / static_cast<float>(m_width);  // 170/320 = 0.531

    // Width percent needed to maintain logo aspect ratio at given height percent
    float widthPercent = params.heightPercent * screenAspectRatio * logoAspectRatio;
    float heightPercent = params.heightPercent;

    // Calculate top-left position (accounting for anchor)
    float topLeftX = params.posX - (params.anchorX * widthPercent);
    float topLeftY = params.posY - (params.anchorY * heightPercent);

    // Convert to pixels
    out_x = m_display->relativeToAbsoluteX(topLeftX);
    out_y = m_display->relativeToAbsoluteY(topLeftY);
    out_width = m_display->relativeToAbsoluteWidth(widthPercent);
    out_height = m_display->relativeToAbsoluteHeight(heightPercent);
}

void LogoScreen::renderLogo() {
    if (m_compositeBuffer == nullptr || m_display == nullptr) return;

    // Calculate current logo bounding box
    int32_t curr_x, curr_y, curr_width, curr_height;
    calculateBoundingBox(m_current, curr_x, curr_y, curr_width, curr_height);

    // Calculate dirty rect (union of old and new bounding boxes)
    int32_t old_left = m_hasDrawnLogo ? m_lastLogoX : curr_x;
    int32_t old_right = m_hasDrawnLogo ? (m_lastLogoX + m_lastLogoWidth) : curr_x;
    int32_t old_top = m_hasDrawnLogo ? m_lastLogoY : curr_y;
    int32_t old_bottom = m_hasDrawnLogo ? (m_lastLogoY + m_lastLogoHeight) : curr_y;

    int32_t new_left = curr_x;
    int32_t new_right = curr_x + curr_width;
    int32_t new_top = curr_y;
    int32_t new_bottom = curr_y + curr_height;

    // Union of both bounding boxes
    int32_t box_x = (old_left < new_left) ? old_left : new_left;
    int32_t box_y = (old_top < new_top) ? old_top : new_top;
    int32_t box_right = (old_right > new_right) ? old_right : new_right;
    int32_t box_bottom = (old_bottom > new_bottom) ? old_bottom : new_bottom;

    // Clamp to screen bounds
    if (box_x < 0) box_x = 0;
    if (box_y < 0) box_y = 0;
    if (box_right >= m_width) box_right = m_width - 1;
    if (box_bottom >= m_height) box_bottom = m_height - 1;

    int32_t box_width = box_right - box_x + 1;
    int32_t box_height = box_bottom - box_y + 1;

    if (box_width <= 0 || box_height <= 0) return;

    // Allocate temp buffer for the dirty region
    size_t buffer_size = static_cast<size_t>(box_width) * static_cast<size_t>(box_height);
    uint16_t* region_buffer = static_cast<uint16_t*>(malloc(buffer_size * sizeof(uint16_t)));
    if (region_buffer == nullptr) return;

    // Step 1: Copy clean background from composite buffer (erases old logo)
    for (int32_t row = 0; row < box_height; row++) {
        int32_t src_y = box_y + row;
        if (src_y < 0 || src_y >= m_height) continue;

        size_t src_offset = static_cast<size_t>(src_y) * static_cast<size_t>(m_width) + static_cast<size_t>(box_x);
        size_t dst_offset = static_cast<size_t>(row) * static_cast<size_t>(box_width);
        memcpy(&region_buffer[dst_offset], &m_compositeBuffer[src_offset], box_width * sizeof(uint16_t));
    }

    // Step 2: Render new logo into the temp buffer
    // Calculate width maintaining aspect ratio and accounting for screen dimensions
    float logoAspectRatio = VectorAssets::Lpadlogo.original_width /
                           VectorAssets::Lpadlogo.original_height;
    float screenAspectRatio = static_cast<float>(m_height) / static_cast<float>(m_width);
    float widthPercent = m_current.heightPercent * screenAspectRatio * logoAspectRatio;

    // For each path in the logo
    for (size_t path_idx = 0; path_idx < VectorAssets::Lpadlogo.num_paths; path_idx++) {
        const VectorPath& path = VectorAssets::Lpadlogo.paths[path_idx];

        // For each triangle in the path
        for (size_t tri_idx = 0; tri_idx < path.num_tris; tri_idx++) {
            const VectorTriangle& tri = path.tris[tri_idx];

            // Transform vertices (same as VectorRenderer::draw)
            float base_x = m_current.posX - (m_current.anchorX * widthPercent);
            float base_y = m_current.posY - (m_current.anchorY * m_current.heightPercent);

            float v1_x_pct = base_x + (tri.v1.x * widthPercent);
            float v1_y_pct = base_y + (tri.v1.y * m_current.heightPercent);
            float v2_x_pct = base_x + (tri.v2.x * widthPercent);
            float v2_y_pct = base_y + (tri.v2.y * m_current.heightPercent);
            float v3_x_pct = base_x + (tri.v3.x * widthPercent);
            float v3_y_pct = base_y + (tri.v3.y * m_current.heightPercent);

            int32_t v1_x = m_display->relativeToAbsoluteX(v1_x_pct);
            int32_t v1_y = m_display->relativeToAbsoluteY(v1_y_pct);
            int32_t v2_x = m_display->relativeToAbsoluteX(v2_x_pct);
            int32_t v2_y = m_display->relativeToAbsoluteY(v2_y_pct);
            int32_t v3_x = m_display->relativeToAbsoluteX(v3_x_pct);
            int32_t v3_y = m_display->relativeToAbsoluteY(v3_y_pct);

            // Rasterize triangle into region buffer (scanline algorithm)
            // Find bounding box of triangle
            int32_t min_x = v1_x < v2_x ? (v1_x < v3_x ? v1_x : v3_x) : (v2_x < v3_x ? v2_x : v3_x);
            int32_t max_x = v1_x > v2_x ? (v1_x > v3_x ? v1_x : v3_x) : (v2_x > v3_x ? v2_x : v3_x);
            int32_t min_y = v1_y < v2_y ? (v1_y < v3_y ? v1_y : v3_y) : (v2_y < v3_y ? v2_y : v3_y);
            int32_t max_y = v1_y > v2_y ? (v1_y > v3_y ? v1_y : v3_y) : (v2_y > v3_y ? v2_y : v3_y);

            // Clip to dirty region
            if (min_x < box_x) min_x = box_x;
            if (max_x > box_right) max_x = box_right;
            if (min_y < box_y) min_y = box_y;
            if (max_y > box_bottom) max_y = box_bottom;

            // Fill triangle using barycentric coordinates
            for (int32_t py = min_y; py <= max_y; py++) {
                for (int32_t px = min_x; px <= max_x; px++) {
                    // Barycentric coordinate test
                    int32_t dx1 = px - v1_x, dy1 = py - v1_y;
                    int32_t dx2 = px - v2_x, dy2 = py - v2_y;
                    int32_t dx3 = px - v3_x, dy3 = py - v3_y;

                    int32_t e1 = (v2_x - v1_x) * dy1 - (v2_y - v1_y) * dx1;
                    int32_t e2 = (v3_x - v2_x) * dy2 - (v3_y - v2_y) * dx2;
                    int32_t e3 = (v1_x - v3_x) * dy3 - (v1_y - v3_y) * dx3;

                    // Point is inside if all edges have same sign
                    if ((e1 >= 0 && e2 >= 0 && e3 >= 0) || (e1 <= 0 && e2 <= 0 && e3 <= 0)) {
                        int32_t buffer_x = px - box_x;
                        int32_t buffer_y = py - box_y;
                        if (buffer_x >= 0 && buffer_x < box_width && buffer_y >= 0 && buffer_y < box_height) {
                            size_t buffer_index = static_cast<size_t>(buffer_y) * static_cast<size_t>(box_width) + static_cast<size_t>(buffer_x);
                            region_buffer[buffer_index] = path.color;
                        }
                    }
                }
            }
        }
    }

    // Step 3: Single atomic blit to display
    hal_display_fast_blit(static_cast<int16_t>(box_x), static_cast<int16_t>(box_y),
                          static_cast<int16_t>(box_width), static_cast<int16_t>(box_height),
                          region_buffer);

    free(region_buffer);

    // Track logo position for next frame
    m_lastLogoX = curr_x;
    m_lastLogoY = curr_y;
    m_lastLogoWidth = curr_width;
    m_lastLogoHeight = curr_height;
    m_hasDrawnLogo = true;
}
