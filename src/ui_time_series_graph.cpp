/**
 * @file ui_time_series_graph.cpp
 * @brief Implementation of Layered Rendering Time Series Graph
 */

#define _USE_MATH_DEFINES
#include "ui_time_series_graph.h"
#include <Arduino_GFX_Library.h>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function to interpolate between two RGB565 colors
static uint16_t interpolate_color(uint16_t color1, uint16_t color2, float t) {
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;

    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;

    uint8_t r = static_cast<uint8_t>(r1 + t * (r2 - r1));
    uint8_t g = static_cast<uint8_t>(g1 + t * (g2 - g1));
    uint8_t b = static_cast<uint8_t>(b1 + t * (b2 - b1));

    return ((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
}

TimeSeriesGraph::TimeSeriesGraph(const GraphTheme& theme, Arduino_GFX* main_display,
                                 int32_t width, int32_t height)
    : theme_(theme), main_display_(main_display), width_(width), height_(height),
      bg_canvas_(nullptr), data_canvas_(nullptr),
      rel_main_(nullptr), rel_bg_(nullptr), rel_data_(nullptr),
      pulse_phase_(0.0f), y_tick_increment_(0.0f),
      cached_y_min_(0.0), cached_y_max_(0.0), range_cached_(false) {
}

TimeSeriesGraph::~TimeSeriesGraph() {
    // Clean up RelativeDisplay instances
    delete rel_main_;
    delete rel_bg_;
    delete rel_data_;

    // Clean up canvas instances (they free their PSRAM buffers)
    delete bg_canvas_;
    delete data_canvas_;
}

bool TimeSeriesGraph::begin() {
#ifdef BOARD_HAS_PSRAM
    // Check if PSRAM is available
    if (psramInit()) {
        // Allocate background canvas in PSRAM
        bg_canvas_ = new Arduino_Canvas(width_, height_, main_display_);
        if (!bg_canvas_ || !bg_canvas_->begin()) {
            delete bg_canvas_;
            bg_canvas_ = nullptr;
            return false;
        }

        // Allocate data canvas in PSRAM
        data_canvas_ = new Arduino_Canvas(width_, height_, main_display_);
        if (!data_canvas_ || !data_canvas_->begin()) {
            delete data_canvas_;
            delete bg_canvas_;
            data_canvas_ = nullptr;
            bg_canvas_ = nullptr;
            return false;
        }

        // Create RelativeDisplay instances for each layer
        rel_main_ = new RelativeDisplay(main_display_, width_, height_);
        rel_bg_ = new RelativeDisplay(bg_canvas_, width_, height_);
        rel_data_ = new RelativeDisplay(data_canvas_, width_, height_);

        return true;
    }
#endif
    return false;
}

void TimeSeriesGraph::setData(const GraphData& data) {
    data_ = data;
    range_cached_ = false;  // Invalidate cached range when data changes
}

void TimeSeriesGraph::setYTicks(float increment) {
    y_tick_increment_ = increment;
}

void TimeSeriesGraph::drawBackground() {
    if (!rel_bg_) return;

    // Fill background canvas with color or gradient
    if (theme_.useBackgroundGradient) {
        // Draw gradient to background canvas using RelativeDisplay
        int32_t x_start = 0;
        int32_t y_start = 0;
        int32_t x_end = width_;
        int32_t y_end = height_;

        float angle_rad = theme_.backgroundGradient.angle_deg * M_PI / 180.0f;
        float dx = cosf(angle_rad);
        float dy = sinf(angle_rad);

        // Simple vertical gradient for performance
        if (fabsf(theme_.backgroundGradient.angle_deg - 90.0f) < 5.0f) {
            for (int32_t y = y_start; y < y_end; y++) {
                float t = static_cast<float>(y - y_start) / static_cast<float>(y_end - y_start);
                uint16_t color;
                if (theme_.backgroundGradient.num_stops == 2) {
                    color = interpolate_color(theme_.backgroundGradient.color_stops[0],
                                            theme_.backgroundGradient.color_stops[1], t);
                } else {
                    if (t < 0.5f) {
                        color = interpolate_color(theme_.backgroundGradient.color_stops[0],
                                                theme_.backgroundGradient.color_stops[1], t * 2.0f);
                    } else {
                        color = interpolate_color(theme_.backgroundGradient.color_stops[1],
                                                theme_.backgroundGradient.color_stops[2], (t - 0.5f) * 2.0f);
                    }
                }
                float y_percent = (static_cast<float>(y) / static_cast<float>(height_)) * 100.0f;
                rel_bg_->drawHorizontalLine(y_percent, 0.0f, 100.0f, color);
            }
        } else {
            // Fallback to solid color for other gradient angles
            rel_bg_->fillRect(0.0f, 0.0f, 100.0f, 100.0f, theme_.backgroundColor);
        }
    } else {
        rel_bg_->fillRect(0.0f, 0.0f, 100.0f, 100.0f, theme_.backgroundColor);
    }

    // Draw axes to background canvas
    drawAxes(rel_bg_);

    // Draw ticks if enabled
    if (y_tick_increment_ > 0.0f) {
        drawYTicks(rel_bg_);
    }
}

void TimeSeriesGraph::drawData() {
    if (!rel_data_) return;

    // Clear data canvas to transparent (black with alpha = 0)
    // Note: Arduino_Canvas doesn't support true transparency, so we use the background color
    rel_data_->fillRect(0.0f, 0.0f, 100.0f, 100.0f, theme_.backgroundColor);

    if (!data_.y_values.empty()) {
        drawDataLine(rel_data_);
    }
}

void TimeSeriesGraph::render() {
    if (!bg_canvas_ || !data_canvas_ || !main_display_) return;

    // Blit background canvas to main display
    bg_canvas_->flush();

    // Blit data canvas on top
    data_canvas_->flush();
}

void TimeSeriesGraph::update(float deltaTime) {
    // Update pulse animation phase
    pulse_phase_ += deltaTime * theme_.liveIndicatorPulseSpeed * 2.0f * M_PI;
    if (pulse_phase_ > 2.0f * M_PI) {
        pulse_phase_ -= 2.0f * M_PI;
    }

    // Draw live indicator directly to main display (after render() has been called)
    drawLiveIndicator();
}

void TimeSeriesGraph::drawAxes(RelativeDisplay* target) {
    float x_min = GRAPH_MARGIN_LEFT;
    float x_max = 100.0f - GRAPH_MARGIN_RIGHT;
    float y_min = GRAPH_MARGIN_TOP;
    float y_max = 100.0f - GRAPH_MARGIN_BOTTOM;

    // Draw Y-axis (left edge)
    target->drawVerticalLine(x_min, y_min, y_max, theme_.axisColor);

    // Draw X-axis (bottom edge)
    target->drawHorizontalLine(y_max, x_min, x_max, theme_.axisColor);
}

void TimeSeriesGraph::drawYTicks(RelativeDisplay* target) {
    if (data_.y_values.empty()) return;

    // Calculate data range
    double y_min = *std::min_element(data_.y_values.begin(), data_.y_values.end());
    double y_max = *std::max_element(data_.y_values.begin(), data_.y_values.end());

    if (y_max - y_min < 0.001) return;

    float x_axis = GRAPH_MARGIN_LEFT;
    float tick_end = x_axis + theme_.tickLength;
    float screen_y_min = GRAPH_MARGIN_TOP;
    float screen_y_max = 100.0f - GRAPH_MARGIN_BOTTOM;

    // Draw tick marks
    for (double tick_value = y_min; tick_value <= y_max; tick_value += y_tick_increment_) {
        float y_screen = mapYToScreen(tick_value, y_min, y_max);
        target->drawHorizontalLine(y_screen, x_axis, tick_end, theme_.tickColor);
    }
}

void TimeSeriesGraph::drawDataLine(RelativeDisplay* target) {
    if (data_.y_values.size() < 2) return;

    // Calculate or use cached data range
    if (!range_cached_) {
        cached_y_min_ = *std::min_element(data_.y_values.begin(), data_.y_values.end());
        cached_y_max_ = *std::max_element(data_.y_values.begin(), data_.y_values.end());
        range_cached_ = true;
    }

    double y_min = cached_y_min_;
    double y_max = cached_y_max_;

    if (y_max - y_min < 0.001) {
        y_max = y_min + 1.0;
    }

    size_t point_count = data_.y_values.size();

    // Draw line segments between consecutive points
    for (size_t i = 1; i < point_count; i++) {
        float x1 = mapXToScreen(i - 1, point_count);
        float y1 = mapYToScreen(data_.y_values[i - 1], y_min, y_max);
        float x2 = mapXToScreen(i, point_count);
        float y2 = mapYToScreen(data_.y_values[i], y_min, y_max);

        // Draw simple line using horizontal/vertical segments approximation
        int32_t x1_px = target->relativeToAbsoluteX(x1);
        int32_t y1_px = target->relativeToAbsoluteY(y1);
        int32_t x2_px = target->relativeToAbsoluteX(x2);
        int32_t y2_px = target->relativeToAbsoluteY(y2);

        // Use Bresenham's line algorithm
        int32_t dx = abs(x2_px - x1_px);
        int32_t dy = abs(y2_px - y1_px);
        int32_t sx = (x1_px < x2_px) ? 1 : -1;
        int32_t sy = (y1_px < y2_px) ? 1 : -1;
        int32_t err = dx - dy;

        int32_t x = x1_px;
        int32_t y = y1_px;

        while (true) {
            float x_pct = (static_cast<float>(x) / static_cast<float>(width_)) * 100.0f;
            float y_pct = (static_cast<float>(y) / static_cast<float>(height_)) * 100.0f;
            target->drawPixel(x_pct, y_pct, theme_.lineColor);

            if (x == x2_px && y == y2_px) break;

            int32_t e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }
}

void TimeSeriesGraph::drawLiveIndicator() {
    if (!rel_main_ || data_.y_values.empty()) return;

    // Calculate or use cached data range
    if (!range_cached_) {
        cached_y_min_ = *std::min_element(data_.y_values.begin(), data_.y_values.end());
        cached_y_max_ = *std::max_element(data_.y_values.begin(), data_.y_values.end());
        range_cached_ = true;
    }

    double y_min = cached_y_min_;
    double y_max = cached_y_max_;

    if (y_max - y_min < 0.001) {
        y_max = y_min + 1.0;
    }

    // Get position of last data point
    size_t last_index = data_.y_values.size() - 1;
    float x = mapXToScreen(last_index, data_.y_values.size());
    float y = mapYToScreen(data_.y_values[last_index], y_min, y_max);

    // Calculate pulsing radius
    float pulse_factor = (sinf(pulse_phase_) + 1.0f) / 2.0f;  // 0 to 1
    float base_radius = 1.0f;  // Base radius in relative %
    float radius = base_radius + (base_radius * 0.5f * pulse_factor);

    // Draw pulsing circle with radial gradient
    int32_t center_x = rel_main_->relativeToAbsoluteX(x);
    int32_t center_y = rel_main_->relativeToAbsoluteY(y);
    int32_t radius_px = static_cast<int32_t>((radius / 100.0f) * ((width_ + height_) / 2.0f));

    for (int32_t dy = -radius_px; dy <= radius_px; dy++) {
        for (int32_t dx = -radius_px; dx <= radius_px; dx++) {
            float dist = sqrtf(static_cast<float>(dx * dx + dy * dy));
            if (dist <= radius_px) {
                float t = dist / static_cast<float>(radius_px);
                uint16_t color = interpolate_color(
                    theme_.liveIndicatorGradient.color_stops[0],
                    theme_.liveIndicatorGradient.color_stops[1],
                    t
                );

                int32_t draw_x = center_x + dx;
                int32_t draw_y = center_y + dy;

                if (draw_x >= 0 && draw_x < width_ && draw_y >= 0 && draw_y < height_) {
                    float x_pct = (static_cast<float>(draw_x) / static_cast<float>(width_)) * 100.0f;
                    float y_pct = (static_cast<float>(draw_y) / static_cast<float>(height_)) * 100.0f;
                    rel_main_->drawPixel(x_pct, y_pct, color);
                }
            }
        }
    }
}

float TimeSeriesGraph::mapYToScreen(double y_value, double y_min, double y_max) {
    float y_range = static_cast<float>(y_max - y_min);
    float normalized = static_cast<float>(y_value - y_min) / y_range;

    float screen_y_min = GRAPH_MARGIN_TOP;
    float screen_y_max = 100.0f - GRAPH_MARGIN_BOTTOM;
    float screen_range = screen_y_max - screen_y_min;

    // Invert Y-axis (higher values at top)
    return screen_y_max - (normalized * screen_range);
}

float TimeSeriesGraph::mapXToScreen(size_t x_index, size_t x_count) {
    float normalized = static_cast<float>(x_index) / static_cast<float>(x_count - 1);

    float screen_x_min = GRAPH_MARGIN_LEFT;
    float screen_x_max = 100.0f - GRAPH_MARGIN_RIGHT;
    float screen_range = screen_x_max - screen_x_min;

    return screen_x_min + (normalized * screen_range);
}
