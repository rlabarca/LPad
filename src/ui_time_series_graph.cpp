/**
 * @file ui_time_series_graph.cpp
 * @brief Implementation of Layered Rendering Time Series Graph
 */

#define _USE_MATH_DEFINES
#include "ui_time_series_graph.h"
#include "../hal/display.h"
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
      composite_buffer_(nullptr), composite_buffer_size_(0),
      pulse_phase_(0.0f), y_tick_increment_(0.0f),
      last_indicator_x_(0), last_indicator_y_(0), last_indicator_radius_(0),
      has_drawn_indicator_(false),
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

    // Clean up composite buffer
    if (composite_buffer_ != nullptr) {
        free(composite_buffer_);
        composite_buffer_ = nullptr;
    }
}

bool TimeSeriesGraph::begin() {
#ifdef BOARD_HAS_PSRAM
    // PSRAM should already be initialized by the HAL or Arduino framework
    // Check if PSRAM is available
    size_t psram_size = ESP.getPsramSize();
    size_t psram_free = ESP.getFreePsram();

    Serial.printf("  [INFO] PSRAM total: %zu bytes\n", psram_size);
    Serial.printf("  [INFO] PSRAM free: %zu bytes\n", psram_free);

    if (psram_size == 0) {
        Serial.println("  [ERROR] No PSRAM detected on this board");
        return false;
    }

    // Calculate required memory (2 canvases * width * height * 2 bytes per pixel)
    size_t required_bytes = 2 * width_ * height_ * 2;
    Serial.printf("  [INFO] Required memory: %zu bytes\n", required_bytes);

    if (psram_free < required_bytes) {
        Serial.printf("  [ERROR] Insufficient PSRAM (need %zu, have %zu)\n",
                     required_bytes, psram_free);
        return false;
    }

    // Allocate background canvas in PSRAM
    Serial.println("  [INFO] Allocating background canvas...");
    bg_canvas_ = new Arduino_Canvas(width_, height_, main_display_);
    if (!bg_canvas_ || !bg_canvas_->begin(GFX_SKIP_OUTPUT_BEGIN)) {
        Serial.println("  [ERROR] Failed to create background canvas");
        delete bg_canvas_;
        bg_canvas_ = nullptr;
        return false;
    }
    Serial.println("  [OK] Background canvas created");

    // Allocate data canvas in PSRAM
    Serial.println("  [INFO] Allocating data canvas...");
    data_canvas_ = new Arduino_Canvas(width_, height_, main_display_);
    if (!data_canvas_ || !data_canvas_->begin(GFX_SKIP_OUTPUT_BEGIN)) {
        Serial.println("  [ERROR] Failed to create data canvas");
        delete data_canvas_;
        delete bg_canvas_;
        data_canvas_ = nullptr;
        bg_canvas_ = nullptr;
        return false;
    }
    Serial.println("  [OK] Data canvas created");

    // Create RelativeDisplay instances for each layer
    Serial.println("  [INFO] Creating RelativeDisplay wrappers...");
    rel_main_ = new RelativeDisplay(main_display_, width_, height_);
    rel_bg_ = new RelativeDisplay(bg_canvas_, width_, height_);
    rel_data_ = new RelativeDisplay(data_canvas_, width_, height_);
    Serial.println("  [OK] RelativeDisplay wrappers created");

    return true;
#else
    Serial.println("  [ERROR] BOARD_HAS_PSRAM not defined");
    return false;
#endif
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

        // Calculate gradient direction vector
        float dx = cosf(angle_rad);
        float dy = sinf(angle_rad);

        // Calculate gradient length (diagonal distance for proper normalization)
        float gradient_length = sqrtf(static_cast<float>(width_ * width_ + height_ * height_));

        // Draw gradient pixel by pixel (could be optimized with line-based rendering)
        for (int32_t py = y_start; py < y_end; py++) {
            for (int32_t px = x_start; px < x_end; px++) {
                // Calculate position along gradient direction
                // Project pixel position onto gradient vector
                float proj = (px * dx + py * dy);

                // Normalize to 0.0 - 1.0 range based on angle
                float t;
                if (fabsf(theme_.backgroundGradient.angle_deg - 90.0f) < 5.0f) {
                    // Vertical gradient (optimized path)
                    t = static_cast<float>(py) / static_cast<float>(height_);
                } else if (fabsf(theme_.backgroundGradient.angle_deg - 0.0f) < 5.0f) {
                    // Horizontal gradient (optimized path)
                    t = static_cast<float>(px) / static_cast<float>(width_);
                } else {
                    // Diagonal gradient - normalize based on projection
                    t = proj / gradient_length;
                }

                // Clamp t to [0, 1]
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;

                // Interpolate color based on number of stops
                uint16_t color;
                if (theme_.backgroundGradient.num_stops == 2) {
                    color = interpolate_color(
                        theme_.backgroundGradient.color_stops[0],
                        theme_.backgroundGradient.color_stops[1],
                        t
                    );
                } else {
                    // 3-color gradient
                    if (t < 0.5f) {
                        color = interpolate_color(
                            theme_.backgroundGradient.color_stops[0],
                            theme_.backgroundGradient.color_stops[1],
                            t * 2.0f
                        );
                    } else {
                        color = interpolate_color(
                            theme_.backgroundGradient.color_stops[1],
                            theme_.backgroundGradient.color_stops[2],
                            (t - 0.5f) * 2.0f
                        );
                    }
                }

                // Draw pixel in relative coordinates
                float x_pct = (static_cast<float>(px) / static_cast<float>(width_)) * 100.0f;
                float y_pct = (static_cast<float>(py) / static_cast<float>(height_)) * 100.0f;
                rel_bg_->drawPixel(x_pct, y_pct, color);
            }
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

    // Clear data canvas using chroma key color for transparency
    // We use 0x0001 (nearly black) as our "transparent" color
    // This will be skipped during compositing
    constexpr uint16_t CHROMA_KEY = 0x0001;
    rel_data_->fillRect(0.0f, 0.0f, 100.0f, 100.0f, CHROMA_KEY);

    if (!data_.y_values.empty()) {
        drawDataLine(rel_data_);
    }
}

void TimeSeriesGraph::render() {
    if (!bg_canvas_ || !data_canvas_ || !main_display_) return;

    // OPTIMIZATION: Pre-composite layers in memory, then do single DMA blit
    // This avoids multiple address window updates and is faster for sparse data

    constexpr uint16_t CHROMA_KEY = 0x0001;
    uint16_t* bg_buffer = bg_canvas_->getFramebuffer();
    uint16_t* data_buffer = data_canvas_->getFramebuffer();

    if (!bg_buffer || !data_buffer) return;

    // Allocate composite buffer in PSRAM if needed
    size_t required_size = static_cast<size_t>(width_) * static_cast<size_t>(height_);

    if (composite_buffer_ == nullptr || composite_buffer_size_ != required_size) {
        if (composite_buffer_ != nullptr) {
            free(composite_buffer_);
        }
        composite_buffer_ = static_cast<uint16_t*>(ps_malloc(required_size * sizeof(uint16_t)));
        composite_buffer_size_ = required_size;
    }

    if (composite_buffer_ == nullptr) {
        // Fallback: blit layers separately if allocation fails
        hal_display_fast_blit(0, 0, width_, height_, bg_buffer);
        hal_display_fast_blit_transparent(0, 0, width_, height_, data_buffer, CHROMA_KEY);
        return;
    }

    // Composite: background + data (with transparency) in memory
    for (size_t i = 0; i < required_size; i++) {
        if (data_buffer[i] != CHROMA_KEY) {
            composite_buffer_[i] = data_buffer[i];  // Use data pixel
        } else {
            composite_buffer_[i] = bg_buffer[i];     // Use background pixel
        }
    }

    // Single fast DMA blit of the composited result
    hal_display_fast_blit(0, 0, width_, height_, composite_buffer_);
}

void TimeSeriesGraph::update(float deltaTime) {
    // Clamp deltaTime to prevent large jumps (max 100ms = 0.1s)
    // This prevents animation glitches if a frame takes too long
    if (deltaTime > 0.1f) {
        deltaTime = 0.1f;
    }

    // Update pulse animation phase smoothly
    // Phase advances continuously from 0 to 2π
    pulse_phase_ += deltaTime * theme_.liveIndicatorPulseSpeed * 2.0f * M_PI;

    // Wrap phase smoothly using fmod for continuous motion
    pulse_phase_ = fmodf(pulse_phase_, 2.0f * M_PI);
    if (pulse_phase_ < 0) {
        pulse_phase_ += 2.0f * M_PI;
    }

    // Erase old indicator by restoring from composite buffer
    eraseOldIndicator();

    // Draw new live indicator directly to main display
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

    // Calculate line thickness in pixels
    float thickness_pct = theme_.lineThickness;  // Percentage
    int32_t thickness_px = static_cast<int32_t>((thickness_pct / 100.0f) * ((width_ + height_) / 2.0f));
    if (thickness_px < 1) thickness_px = 1;
    int32_t half_thickness = thickness_px / 2;

    // Draw thick line segments between consecutive points
    for (size_t i = 1; i < point_count; i++) {
        float x1 = mapXToScreen(i - 1, point_count);
        float y1 = mapYToScreen(data_.y_values[i - 1], y_min, y_max);
        float x2 = mapXToScreen(i, point_count);
        float y2 = mapYToScreen(data_.y_values[i], y_min, y_max);

        // Calculate color for this segment (gradient support)
        uint16_t segment_color;
        if (theme_.useLineGradient && theme_.lineGradient.num_stops >= 2) {
            // Interpolate color based on position along X axis
            float t = static_cast<float>(i - 1) / static_cast<float>(point_count - 1);
            if (theme_.lineGradient.num_stops == 2) {
                segment_color = interpolate_color(
                    theme_.lineGradient.color_stops[0],
                    theme_.lineGradient.color_stops[1],
                    t
                );
            } else {
                // 3-color gradient
                if (t < 0.5f) {
                    segment_color = interpolate_color(
                        theme_.lineGradient.color_stops[0],
                        theme_.lineGradient.color_stops[1],
                        t * 2.0f
                    );
                } else {
                    segment_color = interpolate_color(
                        theme_.lineGradient.color_stops[1],
                        theme_.lineGradient.color_stops[2],
                        (t - 0.5f) * 2.0f
                    );
                }
            }
        } else {
            segment_color = theme_.lineColor;
        }

        // Draw thick line using filled rectangle perpendicular to line direction
        int32_t x1_px = target->relativeToAbsoluteX(x1);
        int32_t y1_px = target->relativeToAbsoluteY(y1);
        int32_t x2_px = target->relativeToAbsoluteX(x2);
        int32_t y2_px = target->relativeToAbsoluteY(y2);

        // For each pixel along the center line, draw perpendicular thickness
        int32_t dx = abs(x2_px - x1_px);
        int32_t dy = abs(y2_px - y1_px);
        int32_t sx = (x1_px < x2_px) ? 1 : -1;
        int32_t sy = (y1_px < y2_px) ? 1 : -1;
        int32_t err = dx - dy;

        int32_t x = x1_px;
        int32_t y = y1_px;

        while (true) {
            // Draw thick point by drawing a small filled circle or square
            for (int32_t ty = -half_thickness; ty <= half_thickness; ty++) {
                for (int32_t tx = -half_thickness; tx <= half_thickness; tx++) {
                    // Simple anti-aliasing: only draw if within circular distance
                    float dist = sqrtf(static_cast<float>(tx * tx + ty * ty));
                    if (dist <= static_cast<float>(half_thickness) + 0.5f) {
                        int32_t px = x + tx;
                        int32_t py = y + ty;
                        if (px >= 0 && px < width_ && py >= 0 && py < height_) {
                            float x_pct = (static_cast<float>(px) / static_cast<float>(width_)) * 100.0f;
                            float y_pct = (static_cast<float>(py) / static_cast<float>(height_)) * 100.0f;
                            target->drawPixel(x_pct, y_pct, segment_color);
                        }
                    }
                }
            }

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

void TimeSeriesGraph::eraseOldIndicator() {
    if (!has_drawn_indicator_ || composite_buffer_ == nullptr) return;

    // Restore pixels from composite buffer where old indicator was drawn
    int32_t x_min = last_indicator_x_ - last_indicator_radius_;
    int32_t x_max = last_indicator_x_ + last_indicator_radius_;
    int32_t y_min = last_indicator_y_ - last_indicator_radius_;
    int32_t y_max = last_indicator_y_ + last_indicator_radius_;

    // Clamp to screen bounds
    if (x_min < 0) x_min = 0;
    if (y_min < 0) y_min = 0;
    if (x_max >= width_) x_max = width_ - 1;
    if (y_max >= height_) y_max = height_ - 1;

    // Restore pixels from composite buffer to main display
    for (int32_t y = y_min; y <= y_max; y++) {
        for (int32_t x = x_min; x <= x_max; x++) {
            // Calculate distance from old center
            int32_t dx = x - last_indicator_x_;
            int32_t dy = y - last_indicator_y_;
            float dist = sqrtf(static_cast<float>(dx * dx + dy * dy));

            // Only restore pixels that were part of the circle
            if (dist <= static_cast<float>(last_indicator_radius_)) {
                size_t buffer_index = static_cast<size_t>(y) * static_cast<size_t>(width_) + static_cast<size_t>(x);
                uint16_t color = composite_buffer_[buffer_index];

                float x_pct = (static_cast<float>(x) / static_cast<float>(width_)) * 100.0f;
                float y_pct = (static_cast<float>(y) / static_cast<float>(height_)) * 100.0f;
                rel_main_->drawPixel(x_pct, y_pct, color);
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

    // Calculate pulsing radius with smooth easing
    // Use smoothstep for natural, continuous animation
    // smoothstep formula: 3t² - 2t³ creates smooth acceleration and deceleration
    float t = (sinf(pulse_phase_) + 1.0f) / 2.0f;  // 0 to 1
    float pulse_factor = t * t * (3.0f - 2.0f * t);  // Smoothstep easing

    // Animate between base size and 20% larger
    // Small variation (20%) is more subtle and professional
    float base_radius = 4.0f;  // Base radius in relative % (increased for visibility)
    float size_variation = 0.20f;  // 20% size change
    float radius = base_radius * (1.0f + size_variation * pulse_factor);

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

    // Track indicator position for next frame's erase
    last_indicator_x_ = center_x;
    last_indicator_y_ = center_y;
    last_indicator_radius_ = radius_px;
    has_drawn_indicator_ = true;
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
