/**
 * @file ui_live_indicator.cpp
 * @brief Animated Live Indicator implementation
 *
 * Provides a reusable pulsing indicator component with radial gradient rendering.
 * See features/ui_live_indicator.md for complete specification.
 */

#define _USE_MATH_DEFINES
#include "ui_live_indicator.h"
#include "gradients.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LiveIndicator::LiveIndicator(const IndicatorTheme& theme, RelativeDisplay* rel_display)
    : theme_(theme), rel_display_(rel_display), pulse_phase_(0.0f) {
}

void LiveIndicator::update(float deltaTime) {
    if (theme_.pulseDuration <= 0.0f) return;

    // Convert pulseDuration from milliseconds to seconds
    float pulse_duration_sec = theme_.pulseDuration / 1000.0f;

    // Calculate pulse speed in cycles per second
    float pulse_speed = 1.0f / pulse_duration_sec;

    // Update phase (radians per second = cycles/sec * 2*PI)
    pulse_phase_ += deltaTime * pulse_speed * 2.0f * static_cast<float>(M_PI);

    // Wrap phase to [0, 2*PI]
    pulse_phase_ = fmodf(pulse_phase_, 2.0f * static_cast<float>(M_PI));
    if (pulse_phase_ < 0.0f) {
        pulse_phase_ += 2.0f * static_cast<float>(M_PI);
    }
}

void LiveIndicator::draw(float x_percent, float y_percent) {
    if (!rel_display_) return;

    float radius = getCurrentRadius();

    // Create radial gradient using existing gradient primitive
    RadialGradient gradient;
    gradient.center_x = x_percent;
    gradient.center_y = y_percent;
    gradient.radius = radius;
    gradient.color_stops[0] = theme_.innerColor;  // Center color
    gradient.color_stops[1] = theme_.outerColor;  // Edge color

    // Use the existing relative display gradient drawing function
    display_relative_fill_circle_gradient(x_percent, y_percent, radius, gradient);
}

float LiveIndicator::getCurrentRadius() const {
    // Calculate pulsing radius with smooth easing
    // Use sine wave and smoothstep for natural, continuous animation
    float t = (sinf(pulse_phase_) + 1.0f) / 2.0f;  // 0 to 1
    float pulse_factor = t * t * (3.0f - 2.0f * t);  // Smoothstep easing

    // Interpolate between min and max radius
    return theme_.minRadius + (theme_.maxRadius - theme_.minRadius) * pulse_factor;
}

void LiveIndicator::reset() {
    pulse_phase_ = 0.0f;
}
