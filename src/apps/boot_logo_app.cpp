/**
 * @file boot_logo_app.cpp
 * @brief Boot Logo Application Implementation
 *
 * State machine: WAIT (2s static) -> ANIMATE (1.5s interpolation) -> DONE (transition)
 * Logo animates from center (75% height) to top-right corner (10% height)
 * using EaseInOutCubic easing, then hands off to the next app.
 *
 * Specification: features/app_boot_logo.md
 */

#include "boot_logo_app.h"
#include "../vector_renderer.h"
#include "../generated/vector_assets.h"
#include "../relative_display.h"
#include "../ui/ui_render_manager.h"
#include "../themes/default/theme_colors.h"

BootLogoApp::BootLogoApp()
    : m_display(nullptr)
    , m_nextApp(nullptr)
    , m_miniLogo(nullptr)
    , m_state(AnimState::WAIT)
    , m_timer(0.0f)
    , m_needsRender(true)
    , m_transitioned(false)
    , m_backgroundDrawn(false)
    , m_x_percent(START_X_PERCENT)
    , m_y_percent(START_Y_PERCENT)
    , m_width_percent(0.0f)
    , m_anchor_x(START_ANCHOR_X)
    , m_anchor_y(START_ANCHOR_Y)
    , m_prevX(0), m_prevY(0), m_prevW(0), m_prevH(0)
    , m_hasPrevBounds(false)
{
}

BootLogoApp::~BootLogoApp() {
}

bool BootLogoApp::begin(RelativeDisplay* display, AppComponent* nextApp,
                        SystemComponent* miniLogo) {
    if (display == nullptr || nextApp == nullptr) return false;
    m_display = display;
    m_nextApp = nextApp;
    m_miniLogo = miniLogo;

    // Calculate initial width from start height
    m_width_percent = heightToWidthPercent(START_HEIGHT_PERCENT);

    return true;
}

void BootLogoApp::onRun() {
    m_state = AnimState::WAIT;
    m_timer = 0.0f;
    m_needsRender = true;
    m_transitioned = false;
    m_backgroundDrawn = false;
    m_hasPrevBounds = false;
    m_x_percent = START_X_PERCENT;
    m_y_percent = START_Y_PERCENT;
    m_width_percent = heightToWidthPercent(START_HEIGHT_PERCENT);
    m_anchor_x = START_ANCHOR_X;
    m_anchor_y = START_ANCHOR_Y;
}

void BootLogoApp::render() {
    if (m_display == nullptr || m_transitioned) return;

    if (!m_needsRender) return;

    Arduino_GFX* gfx = m_display->getGfx();

    if (!m_backgroundDrawn) {
        // First frame: full background fill (once)
        m_display->drawSolidBackground(LPad::THEME_BACKGROUND);
        m_backgroundDrawn = true;
    } else if (m_hasPrevBounds) {
        // Subsequent frames: erase only the previous logo position
        gfx->fillRect(m_prevX, m_prevY, m_prevW, m_prevH,
                      LPad::THEME_BACKGROUND);
    }

    // Draw logo at current animated position
    VectorRenderer::draw(*m_display, VectorAssets::Lpadlogo,
                        m_x_percent, m_y_percent, m_width_percent,
                        m_anchor_x, m_anchor_y);

    // Store current bounding box for next frame's dirty-rect erase
    calculateLogoBounds(m_prevX, m_prevY, m_prevW, m_prevH);
    m_hasPrevBounds = true;

    m_needsRender = false;
}

void BootLogoApp::update(float dt) {
    if (m_transitioned) return;

    m_timer += dt;

    switch (m_state) {
        case AnimState::WAIT:
            if (m_timer >= WAIT_DURATION) {
                m_state = AnimState::ANIMATE;
                m_timer = 0.0f;
            }
            break;

        case AnimState::ANIMATE: {
            float t = m_timer / ANIMATE_DURATION;
            if (t >= 1.0f) t = 1.0f;

            float eased = easeInOutCubic(t);

            // Interpolate position
            float endX, endY;
            calculateEndPosition(endX, endY);

            m_x_percent = START_X_PERCENT + (endX - START_X_PERCENT) * eased;
            m_y_percent = START_Y_PERCENT + (endY - START_Y_PERCENT) * eased;

            // Interpolate size (height percent -> width percent)
            float currentHeight = START_HEIGHT_PERCENT +
                (END_HEIGHT_PERCENT - START_HEIGHT_PERCENT) * eased;
            m_width_percent = heightToWidthPercent(currentHeight);

            // Interpolate anchor: center (0.5, 0.5) -> top-right (1.0, 0.0)
            m_anchor_x = START_ANCHOR_X + (1.0f - START_ANCHOR_X) * eased;
            m_anchor_y = START_ANCHOR_Y + (0.0f - START_ANCHOR_Y) * eased;

            m_needsRender = true;

            if (t >= 1.0f) {
                m_state = AnimState::DONE;
            }
            break;
        }

        case AnimState::DONE:
            // Show the mini logo overlay before transitioning
            if (m_miniLogo) {
                m_miniLogo->show();
            }

            m_transitioned = true;
            UIRenderManager::getInstance().setActiveApp(m_nextApp);
            break;
    }
}

bool BootLogoApp::handleInput(const touch_gesture_event_t& event) {
    (void)event;
    return false;
}

float BootLogoApp::easeInOutCubic(float t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float f = (2.0f * t) - 2.0f;
        return 0.5f * f * f * f + 1.0f;
    }
}

float BootLogoApp::heightToWidthPercent(float heightPercent) const {
    if (m_display == nullptr) return 0.0f;

    // Logo aspect ratio: width / height
    float logoAspect = VectorAssets::Lpadlogo.original_width /
                       VectorAssets::Lpadlogo.original_height;

    // Screen aspect ratio: height / width (for coordinate system conversion)
    float screenAspect = static_cast<float>(m_display->getHeight()) /
                         static_cast<float>(m_display->getWidth());

    return heightPercent * screenAspect * logoAspect;
}

void BootLogoApp::calculateEndPosition(float& x, float& y) const {
    if (m_display == nullptr) {
        x = 100.0f;
        y = 0.0f;
        return;
    }

    float offsetX = (CORNER_OFFSET_PX / static_cast<float>(m_display->getWidth())) * 100.0f;
    float offsetY = (CORNER_OFFSET_PX / static_cast<float>(m_display->getHeight())) * 100.0f;

    // Top-right corner with buffer
    x = 100.0f - offsetX;
    y = offsetY;
}

void BootLogoApp::calculateLogoBounds(int16_t& x, int16_t& y,
                                      int16_t& w, int16_t& h) const {
    // Replicate VectorRenderer's coordinate math to compute the pixel
    // bounding box of the logo at its current animated position.
    float shapeAspect = VectorAssets::Lpadlogo.original_height /
                        VectorAssets::Lpadlogo.original_width;
    float screenAspect = static_cast<float>(m_display->getWidth()) /
                         static_cast<float>(m_display->getHeight());

    float targetWidth = m_width_percent;
    float targetHeight = m_width_percent * shapeAspect * screenAspect;

    float baseX = m_x_percent - (m_anchor_x * targetWidth);
    float baseY = m_y_percent - (m_anchor_y * targetHeight);

    int32_t px1 = m_display->relativeToAbsoluteX(baseX);
    int32_t py1 = m_display->relativeToAbsoluteY(baseY);
    int32_t px2 = m_display->relativeToAbsoluteX(baseX + targetWidth);
    int32_t py2 = m_display->relativeToAbsoluteY(baseY + targetHeight);

    // Pad by 2px to cover rounding in triangle rasterization
    static const int16_t PAD = 2;
    x = static_cast<int16_t>(px1) - PAD;
    y = static_cast<int16_t>(py1) - PAD;
    w = static_cast<int16_t>(px2 - px1) + PAD * 2;
    h = static_cast<int16_t>(py2 - py1) + PAD * 2;

    // Clamp to screen bounds
    int16_t sw = static_cast<int16_t>(m_display->getWidth());
    int16_t sh = static_cast<int16_t>(m_display->getHeight());
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > sw) w = sw - x;
    if (y + h > sh) h = sh - y;
}
