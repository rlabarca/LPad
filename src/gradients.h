/**
 * @file gradients.h
 * @brief Common gradient structure definitions
 *
 * This header provides shared gradient type definitions used across
 * multiple modules (RelativeDisplay, TimeSeriesGraph, etc.).
 * It has no dependencies on hardware-specific headers.
 */

#ifndef GRADIENTS_H
#define GRADIENTS_H

#include <stdint.h>
#include <cstddef>

/**
 * @struct LinearGradient
 * @brief Defines a linear color gradient
 */
struct LinearGradient {
    float angle_deg;               ///< Gradient angle in degrees (0=left-to-right, 90=top-to-bottom)
    uint16_t color_stops[3];       ///< Up to 3 color values (RGB565)
    size_t num_stops;              ///< Number of color stops (2-3)
};

/**
 * @struct RadialGradient
 * @brief Defines a radial color gradient
 */
struct RadialGradient {
    float center_x;                ///< Center X coordinate (relative percentage)
    float center_y;                ///< Center Y coordinate (relative percentage)
    float radius;                  ///< Radius in relative percentage units
    uint16_t color_stops[2];       ///< Inner and outer color values (RGB565)
};

#endif // GRADIENTS_H
