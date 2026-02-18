/**
 * @file boot_logo_app.cpp
 * @brief Boot Logo Application Implementation
 *
 * State machine: WAIT (2s) -> ANIMATE (1.5s) -> HOLDING (wait for signal) -> DONE | ERROR
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
#include "../theme_manager.h"
#include "../themes/default/theme_colors.h"
#include "../../hal/display.h"
#include <Arduino_GFX_Library.h>

BootLogoApp::BootLogoApp()
    : m_bootComplete(false)
    , m_hasError(false)
    , m_errorMessage(nullptr)
    , m_errorRendered(false)
    , m_display(nullptr)
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
    m_errorRendered = false;
    m_x_percent = START_X_PERCENT;
    m_y_percent = START_Y_PERCENT;
    m_width_percent = heightToWidthPercent(START_HEIGHT_PERCENT);
    m_anchor_x = START_ANCHOR_X;
    m_anchor_y = START_ANCHOR_Y;
}

void BootLogoApp::render() {
    if (m_display == nullptr || m_transitioned) return;

    // Error state: one-shot render of centered error message
    if (m_state == AnimState::ERROR) {
        if (!m_errorRendered) {
            renderErrorScreen();
            m_errorRendered = true;
        }
        return;
    }

    if (!m_needsRender) return;

    if (!m_backgroundDrawn) {
        // First frame: full background fill + draw logo directly.
        // No flashing risk since screen is freshly cleared.
        m_display->drawSolidBackground(LPad::THEME_BACKGROUND);
        VectorRenderer::draw(*m_display, VectorAssets::Lpadlogo,
                            m_x_percent, m_y_percent, m_width_percent,
                            m_anchor_x, m_anchor_y);
        calculateLogoBounds(m_prevX, m_prevY, m_prevW, m_prevH);
        m_hasPrevBounds = true;
        m_backgroundDrawn = true;
    } else {
        // Animation frames: compose erase + draw into a single canvas,
        // then blit atomically to prevent flashing.
        renderAtomicFrame();
    }

    m_needsRender = false;
}

void BootLogoApp::update(float dt) {
    if (m_transitioned) return;

    // Highest priority: error from background task overrides all states
    if (m_hasError && m_state != AnimState::ERROR) {
        m_state = AnimState::ERROR;
        return;
    }

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
                m_state = AnimState::HOLDING;
            }
            break;
        }

        case AnimState::HOLDING:
            // Wait for background task to signal boot complete
            if (m_bootComplete) {
                m_state = AnimState::DONE;
            }
            break;

        case AnimState::DONE:
            // Show the mini logo overlay before transitioning
            if (m_miniLogo) {
                m_miniLogo->show();
            }

            m_transitioned = true;
            UIRenderManager::getInstance().setActiveApp(m_nextApp);
            break;

        case AnimState::ERROR:
            // Halted — do nothing
            break;
    }
}

bool BootLogoApp::handleInput(const touch_gesture_event_t& event) {
    (void)event;
    return false;
}

void BootLogoApp::setBootComplete() {
    m_bootComplete = true;
}

void BootLogoApp::setErrorMessage(const char* message) {
    m_errorMessage = message;  // Set pointer first (string literal in flash)
    m_hasError = true;         // Then set flag (reader checks flag first)
}

void BootLogoApp::renderErrorScreen() {
    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();
    m_display->drawSolidBackground(theme->colors.background);

    Arduino_GFX* gfx = m_display->getGfx();
    gfx->setFont(static_cast<const GFXfont*>(theme->fonts.normal));
    gfx->setTextColor(theme->colors.text_main);

    const char* msg = m_errorMessage ? m_errorMessage : "Unknown Error";
    int16_t x1, y1;
    uint16_t tw, th;
    gfx->getTextBounds(msg, 0, 0, &x1, &y1, &tw, &th);
    int16_t cx = (m_display->getWidth() - tw) / 2 - x1;
    int16_t cy = (m_display->getHeight() - th) / 2 - y1;
    gfx->setCursor(cx, cy);
    gfx->print(msg);
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

void BootLogoApp::renderAtomicFrame() {
    // Atomic compose-and-blit: same pattern as the live indicator in
    // TimeSeriesGraph::drawLiveIndicator(). Erase + draw are composed
    // into a single off-screen canvas, then blitted in one operation
    // so the display never shows the intermediate erased state.

    // 1. Calculate new logo bounds
    int16_t newX, newY, newW, newH;
    calculateLogoBounds(newX, newY, newW, newH);

    // 2. Union of old and new bounding boxes
    int16_t unionX, unionY, unionW, unionH;
    if (m_hasPrevBounds) {
        int16_t prevRight = m_prevX + m_prevW;
        int16_t prevBottom = m_prevY + m_prevH;
        int16_t newRight = newX + newW;
        int16_t newBottom = newY + newH;

        unionX = (m_prevX < newX) ? m_prevX : newX;
        unionY = (m_prevY < newY) ? m_prevY : newY;
        int16_t unionRight = (prevRight > newRight) ? prevRight : newRight;
        int16_t unionBottom = (prevBottom > newBottom) ? prevBottom : newBottom;
        unionW = unionRight - unionX;
        unionH = unionBottom - unionY;
    } else {
        unionX = newX;
        unionY = newY;
        unionW = newW;
        unionH = newH;
    }

    if (unionW <= 0 || unionH <= 0) return;

    // 3. Create off-screen canvas for the union region
    hal_canvas_handle_t canvas = hal_display_canvas_create(unionW, unionH);
    if (canvas == nullptr) return;

    // 4. Fill canvas with background color (this "erases" the old logo)
    hal_display_canvas_fill(canvas, LPad::THEME_BACKGROUND);

    // 5. Draw logo triangles into the canvas at the correct offset.
    //    Replicates VectorRenderer's coordinate math but renders into
    //    the local canvas instead of the main display.
    Arduino_Canvas* canvasPtr = static_cast<Arduino_Canvas*>(canvas);
    const VectorShape& shape = VectorAssets::Lpadlogo;
    float shapeAspect = shape.original_height / shape.original_width;
    float screenAspect = static_cast<float>(m_display->getWidth()) /
                         static_cast<float>(m_display->getHeight());

    float targetWidth = m_width_percent;
    float targetHeight = m_width_percent * shapeAspect * screenAspect;

    float baseX = m_x_percent - (m_anchor_x * targetWidth);
    float baseY = m_y_percent - (m_anchor_y * targetHeight);

    for (size_t pi = 0; pi < shape.num_paths; pi++) {
        const VectorPath& path = shape.paths[pi];
        for (size_t ti = 0; ti < path.num_tris; ti++) {
            const VectorTriangle& tri = path.tris[ti];

            // Absolute pixel coords minus canvas origin
            int32_t x1 = m_display->relativeToAbsoluteX(baseX + tri.v1.x * targetWidth) - unionX;
            int32_t y1 = m_display->relativeToAbsoluteY(baseY + tri.v1.y * targetHeight) - unionY;
            int32_t x2 = m_display->relativeToAbsoluteX(baseX + tri.v2.x * targetWidth) - unionX;
            int32_t y2 = m_display->relativeToAbsoluteY(baseY + tri.v2.y * targetHeight) - unionY;
            int32_t x3 = m_display->relativeToAbsoluteX(baseX + tri.v3.x * targetWidth) - unionX;
            int32_t y3 = m_display->relativeToAbsoluteY(baseY + tri.v3.y * targetHeight) - unionY;

            canvasPtr->fillTriangle(x1, y1, x2, y2, x3, y3, path.color);
        }
    }

    // 6. Single atomic blit — display never sees the erased state
    uint16_t* fb = canvasPtr->getFramebuffer();
    if (fb) {
        hal_display_fast_blit(unionX, unionY, unionW, unionH, fb);
    }

    // 7. Cleanup and update bounds for next frame
    hal_display_canvas_delete(canvas);
    m_prevX = newX;
    m_prevY = newY;
    m_prevW = newW;
    m_prevH = newH;
}
