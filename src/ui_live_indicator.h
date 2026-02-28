/**
 * @file ui_live_indicator.h
 * @brief Animated Live Indicator UI Component
 *
 * This module provides a reusable UI component that represents a "live" status
 * or a current data point. It renders as a circle with a radial gradient and
 * supports a pulsating animation.
 *
 * See features/ui_live_indicator.md for complete specification.
 */

#ifndef UI_LIVE_INDICATOR_H
#define UI_LIVE_INDICATOR_H

#include "relative_display.h"
#include <stdint.h>

/**
 * @struct IndicatorTheme
 * @brief Visual style configuration for the live indicator
 */
struct IndicatorTheme {
    uint16_t innerColor;      ///< Color at the center of the indicator (RGB565)
    uint16_t outerColor;      ///< Color at the edge of the indicator (RGB565)
    float minRadius;          ///< Smallest radius during pulse cycle (relative percentage)
    float maxRadius;          ///< Largest radius during pulse cycle (relative percentage)
    float pulseDuration;      ///< Time in milliseconds for a full grow/shrink cycle
};

/**
 * @class LiveIndicator
 * @brief Animated live indicator component with radial gradient and pulsing animation
 *
 * This class provides a reusable indicator that can be drawn at any position
 * and animates with a smooth pulsing effect.
 */
class LiveIndicator {
public:
    /**
     * @brief Constructor
     * @param theme Visual style configuration
     * @param rel_display Pointer to RelativeDisplay for drawing operations
     */
    LiveIndicator(const IndicatorTheme& theme, RelativeDisplay* rel_display);

    /**
     * @brief Update the animation state
     * @param deltaTime Time elapsed since last update in seconds
     */
    void update(float deltaTime);

    /**
     * @brief Draw the indicator at the specified position
     * @param x_percent X position in relative percentage (0-100)
     * @param y_percent Y position in relative percentage (0-100)
     */
    void draw(float x_percent, float y_percent);

    /**
     * @brief Get the current radius of the indicator
     * @return Current radius in relative percentage units
     */
    float getCurrentRadius() const;

    /**
     * @brief Reset the animation to the starting phase
     */
    void reset();

private:
    IndicatorTheme theme_;
    RelativeDisplay* rel_display_;
    float pulse_phase_;  ///< Current phase of pulse animation (0 to 2*PI)
};

#endif // UI_LIVE_INDICATOR_H
