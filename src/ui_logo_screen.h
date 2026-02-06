#pragma once

#include "relative_display.h"
#include "generated/vector_assets.h"

/**
 * LogoScreen - Manages the startup logo animation
 *
 * State machine:
 * 1. WAIT - Logo displayed large and centered for initial duration
 * 2. ANIMATE - Logo smoothly transitions to top-right corner while shrinking
 * 3. DONE - Animation complete
 */
class LogoScreen {
public:
    enum class State {
        WAIT,
        ANIMATE,
        DONE
    };

    /**
     * Constructor
     * @param waitDuration How long to show logo centered (seconds)
     * @param animDuration How long the shrink/move animation takes (seconds)
     */
    LogoScreen(float waitDuration = 2.0f, float animDuration = 1.5f);

    /**
     * Initialize the screen
     */
    void init();

    /**
     * Update animation state
     * @param deltaTime Time since last frame (seconds)
     * @return Current state
     */
    State update(float deltaTime);

    /**
     * Draw the logo at current position/scale
     * @param display RelativeDisplay to draw to
     * @param backgroundColor Background color to clear with
     */
    void draw(RelativeDisplay& display, uint16_t backgroundColor);

    /**
     * Check if animation is complete
     */
    bool isDone() const { return m_state == State::DONE; }

    /**
     * Get current state
     */
    State getState() const { return m_state; }

private:
    // Timing
    float m_waitDuration;
    float m_animDuration;
    float m_timer;

    // State
    State m_state;

    // Animation parameters
    struct AnimParams {
        float posX;
        float posY;
        float heightPercent;
        float anchorX;
        float anchorY;
    };

    AnimParams m_current;

    // Easing function (EaseInOutCubic)
    static float easeInOutCubic(float t);

    // Calculate interpolated parameters
    void updateAnimParams(float t);
};
