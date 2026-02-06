#pragma once

#include "relative_display.h"
#include "generated/vector_assets.h"
#include <Arduino_GFX_Library.h>

/**
 * LogoScreen - Manages the startup logo animation with flicker-free rendering
 *
 * State machine:
 * 1. WAIT - Logo displayed large and centered for initial duration
 * 2. ANIMATE - Logo smoothly transitions to top-right corner while shrinking
 * 3. DONE - Animation complete
 *
 * Uses dirty-rect optimization with background composite buffer to prevent
 * flicker during animation (similar to TimeSeriesGraph's live indicator).
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
     * Destructor - frees composite buffer
     */
    ~LogoScreen();

    /**
     * Initialize the screen
     * @param display RelativeDisplay to use for rendering
     * @param backgroundColor Background color to use
     * @return true if initialization succeeded
     */
    bool begin(RelativeDisplay* display, uint16_t backgroundColor);

    /**
     * Update animation state and render to display
     * @param deltaTime Time since last frame (seconds)
     * @return Current state
     */
    State update(float deltaTime);

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

    // Display references
    RelativeDisplay* m_display;
    Arduino_GFX* m_gfx;
    int32_t m_width;
    int32_t m_height;
    uint16_t m_backgroundColor;

    // Background composite buffer (for dirty-rect optimization)
    uint16_t* m_compositeBuffer;

    // Dirty-rect tracking
    bool m_hasDrawnLogo;
    int32_t m_lastLogoX;
    int32_t m_lastLogoY;
    int32_t m_lastLogoWidth;
    int32_t m_lastLogoHeight;

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

    // Render logo with dirty-rect optimization
    void renderLogo();

    // Calculate bounding box for logo at given params
    void calculateBoundingBox(const AnimParams& params,
                             int32_t& out_x, int32_t& out_y,
                             int32_t& out_width, int32_t& out_height);
};
