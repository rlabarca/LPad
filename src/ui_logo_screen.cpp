#include "ui_logo_screen.h"
#include "vector_renderer.h"
#include <algorithm>

// Start and end states from spec
static constexpr float START_POS_X = 50.0f;
static constexpr float START_POS_Y = 50.0f;
static constexpr float START_HEIGHT = 75.0f;  // 75% of screen height
static constexpr float START_ANCHOR_X = 0.5f;
static constexpr float START_ANCHOR_Y = 0.5f;

static constexpr float END_POS_X = 100.0f;
static constexpr float END_POS_Y = 0.0f;
static constexpr float END_HEIGHT = 5.0f;  // 5% of screen height
static constexpr float END_ANCHOR_X = 1.0f;
static constexpr float END_ANCHOR_Y = 0.0f;

LogoScreen::LogoScreen(float waitDuration, float animDuration)
    : m_waitDuration(waitDuration)
    , m_animDuration(animDuration)
    , m_timer(0.0f)
    , m_state(State::WAIT)
{
    init();
}

void LogoScreen::init() {
    m_state = State::WAIT;
    m_timer = 0.0f;

    // Set initial position (centered, large)
    m_current.posX = START_POS_X;
    m_current.posY = START_POS_Y;
    m_current.heightPercent = START_HEIGHT;
    m_current.anchorX = START_ANCHOR_X;
    m_current.anchorY = START_ANCHOR_Y;
}

LogoScreen::State LogoScreen::update(float deltaTime) {
    if (m_state == State::DONE) {
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
            break;

        case State::DONE:
            break;
    }

    return m_state;
}

void LogoScreen::draw(RelativeDisplay& display, uint16_t backgroundColor) {
    // Clear background
    display.drawSolidBackground(backgroundColor);

    // Calculate width based on height and aspect ratio
    float aspectRatio = VectorAssets::Lpadlogo.original_width /
                       VectorAssets::Lpadlogo.original_height;
    float widthPercent = m_current.heightPercent * aspectRatio;

    // Draw logo at current position/scale
    VectorRenderer::draw(
        display,
        VectorAssets::Lpadlogo,
        m_current.posX,
        m_current.posY,
        widthPercent,
        m_current.anchorX,
        m_current.anchorY
    );
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
    // Linear interpolation of all parameters
    m_current.posX = START_POS_X + (END_POS_X - START_POS_X) * t;
    m_current.posY = START_POS_Y + (END_POS_Y - START_POS_Y) * t;
    m_current.heightPercent = START_HEIGHT + (END_HEIGHT - START_HEIGHT) * t;
    m_current.anchorX = START_ANCHOR_X + (END_ANCHOR_X - START_ANCHOR_X) * t;
    m_current.anchorY = START_ANCHOR_Y + (END_ANCHOR_Y - START_ANCHOR_Y) * t;
}
