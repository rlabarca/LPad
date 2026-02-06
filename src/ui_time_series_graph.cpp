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
      tick_label_position_(TickLabelPosition::OUTSIDE),
      x_axis_title_(nullptr), y_axis_title_(nullptr),
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

void TimeSeriesGraph::setTheme(const GraphTheme& theme) {
    theme_ = theme;
}

void TimeSeriesGraph::setTickLabelPosition(TickLabelPosition pos) {
    tick_label_position_ = pos;
}

void TimeSeriesGraph::setXAxisTitle(const char* title) {
    x_axis_title_ = title;
}

void TimeSeriesGraph::setYAxisTitle(const char* title) {
    y_axis_title_ = title;
}

TimeSeriesGraph::GraphMargins TimeSeriesGraph::getMargins() const {
    GraphMargins m;
    if (tick_label_position_ == TickLabelPosition::OUTSIDE) {
        m.left = 12.0f;
        m.bottom = 12.0f;
        m.top = 5.0f;
        m.right = 5.0f;
        if (y_axis_title_) m.left += 4.0f;
        if (x_axis_title_) m.bottom += 4.0f;
    } else {
        m.left = 3.0f;
        m.bottom = 3.0f;
        m.top = 3.0f;
        m.right = 3.0f;
    }
    return m;
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
            if (py % 20 == 0) yield();  // Feed watchdog every 20 rows
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

    // Draw Y-axis ticks and labels if enabled
    if (y_tick_increment_ > 0.0f) {
        drawYTicks(rel_bg_);
    }

    // Draw X-axis ticks and labels
    drawXTicks(rel_bg_);

    // Draw axis titles if set
    drawAxisTitles(rel_bg_);
}

void TimeSeriesGraph::drawAxisTitles(RelativeDisplay* target) {
    Arduino_GFX* canvas = target->getGfx();
    if (!canvas) return;

    GraphMargins m = getMargins();

    canvas->setTextColor(theme_.tickColor);
    canvas->setTextSize(1);  // Built-in font for now

    // X-axis title: centered horizontally below X-axis tick labels
    if (x_axis_title_) {
        int16_t x1, y1;
        uint16_t w, h;
        canvas->getTextBounds(x_axis_title_, 0, 0, &x1, &y1, &w, &h);

        float graph_center_x = m.left + (100.0f - m.left - m.right) / 2.0f;
        int32_t center_px = target->relativeToAbsoluteX(graph_center_x);
        int32_t title_x = center_px - static_cast<int32_t>(w) / 2;
        int32_t title_y = height_ - 2;  // 2px from bottom edge

        if (title_x < 0) title_x = 0;
        if (title_x + static_cast<int32_t>(w) >= width_) title_x = width_ - w - 1;
        if (title_y >= height_) title_y = height_ - 1;

        canvas->setCursor(title_x, title_y);
        canvas->print(x_axis_title_);
    }

    // Y-axis title: character by character, vertically centered
    if (y_axis_title_) {
        int len = static_cast<int>(strlen(y_axis_title_));
        if (len == 0 || len > 32) return;

        // Measure reference character for spacing (built-in font is 6x8)
        int32_t char_h = 8;
        int32_t char_w = 6;
        int32_t char_spacing = char_h + 2;
        int32_t total_height = len * char_spacing;

        // Center vertically in graph area
        float graph_center_y = (m.top + (100.0f - m.bottom)) / 2.0f;
        int32_t center_py = target->relativeToAbsoluteY(graph_center_y);
        int32_t start_y = center_py - total_height / 2;
        int32_t title_x = 2;  // 2px from left edge

        for (int i = 0; i < len; i++) {
            int32_t char_y = start_y + i * char_spacing;

            if (char_y >= 0 && char_y < height_ && title_x >= 0 && title_x < width_) {
                canvas->setCursor(title_x, char_y);
                canvas->write(static_cast<uint8_t>(y_axis_title_[i]));
            }
        }
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

    // Draw new live indicator (combines erase + draw in single blit to prevent tearing)
    drawLiveIndicator();
}

void TimeSeriesGraph::drawAxes(RelativeDisplay* target) {
    GraphMargins m = getMargins();
    float x_min = m.left;
    float x_max = 100.0f - m.right;
    float y_min = m.top;
    float y_max = 100.0f - m.bottom;

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

    GraphMargins m = getMargins();
    float x_axis = m.left;

    Arduino_GFX* canvas = target->getGfx();
    if (!canvas) return;

    canvas->setTextColor(theme_.tickColor);
    canvas->setTextSize(1);

    // Draw tick marks and labels (skip first tick at y_min to avoid X-axis overlap)
    for (double tick_value = y_min + y_tick_increment_; tick_value <= y_max; tick_value += y_tick_increment_) {
        float y_screen = mapYToScreen(tick_value, y_min, y_max);

        char label[16];
        snprintf(label, sizeof(label), "%.3f", tick_value);

        int16_t x1, y1;
        uint16_t w, h;
        canvas->getTextBounds(label, 0, 0, &x1, &y1, &w, &h);

        if (tick_label_position_ == TickLabelPosition::OUTSIDE) {
            // Tick extends LEFT from Y-axis
            float tick_start = x_axis - theme_.tickLength;
            target->drawHorizontalLine(y_screen, tick_start, x_axis, theme_.tickColor);

            // Label to LEFT of tick
            int32_t label_x = target->relativeToAbsoluteX(tick_start) - w - 2;
            int32_t label_y = target->relativeToAbsoluteY(y_screen);
            if (label_x < 0) label_x = 0;
            canvas->setCursor(label_x, label_y + h / 2);
            canvas->print(label);
        } else {
            // INSIDE: tick extends RIGHT into graph
            float tick_end = x_axis + theme_.tickLength;
            target->drawHorizontalLine(y_screen, x_axis, tick_end, theme_.tickColor);

            // Label to RIGHT of tick (inside graph)
            int32_t label_x = target->relativeToAbsoluteX(tick_end + 0.5f);
            int32_t label_y = target->relativeToAbsoluteY(y_screen);
            canvas->setCursor(label_x, label_y + h / 2);
            canvas->print(label);
        }
    }
}

void TimeSeriesGraph::drawXTicks(RelativeDisplay* target) {
    if (data_.x_values.empty()) return;

    GraphMargins mx = getMargins();
    float y_axis = 100.0f - mx.bottom;

    Arduino_GFX* canvas = target->getGfx();
    if (!canvas) return;

    canvas->setTextColor(theme_.tickColor);
    canvas->setTextSize(1);

    size_t num_points = data_.x_values.size();
    if (num_points < 2) return;

    size_t tick_interval = (num_points > 5) ? (num_points / 5) : 1;

    // Skip first tick near Y-axis
    for (size_t i = tick_interval; i < num_points; i += tick_interval) {
        float x_screen = mapXToScreen(i, num_points);

        char label[16];
        if (i < data_.x_values.size()) {
            long timestamp = data_.x_values[i];
            snprintf(label, sizeof(label), "%ld", timestamp % 1000);
        } else {
            snprintf(label, sizeof(label), "%zu", i);
        }

        int16_t x1, y1;
        uint16_t w, h;
        canvas->getTextBounds(label, 0, 0, &x1, &y1, &w, &h);

        if (tick_label_position_ == TickLabelPosition::OUTSIDE) {
            // Tick extends DOWN from X-axis
            float tick_end = y_axis + theme_.tickLength;
            target->drawVerticalLine(x_screen, y_axis, tick_end, theme_.tickColor);

            // Label below tick
            int32_t label_x = target->relativeToAbsoluteX(x_screen) - w / 2;
            int32_t label_y = target->relativeToAbsoluteY(tick_end + 0.5f);
            if (label_x < 0) label_x = 0;
            if (label_x + w >= width_) label_x = width_ - w - 1;
            canvas->setCursor(label_x, label_y + h);
            canvas->print(label);
        } else {
            // INSIDE: tick extends UP into graph
            float tick_top = y_axis - theme_.tickLength;
            target->drawVerticalLine(x_screen, tick_top, y_axis, theme_.tickColor);

            // Label above tick (inside graph)
            int32_t label_x = target->relativeToAbsoluteX(x_screen) - w / 2;
            int32_t label_y = target->relativeToAbsoluteY(tick_top) - 2;
            if (label_x < 0) label_x = 0;
            canvas->setCursor(label_x, label_y);
            canvas->print(label);
        }
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

    // Animate from 1 pixel to larger size for clear visibility
    // Calculate 1 pixel in relative percentage
    float avg_dimension = (static_cast<float>(width_) + static_cast<float>(height_)) / 2.0f;
    float one_pixel_pct = (1.0f / avg_dimension) * 100.0f;
    float max_radius = 6.0f;  // Maximum radius in relative %

    // Pulse from 1 pixel to max_radius
    float radius = one_pixel_pct + (max_radius - one_pixel_pct) * pulse_factor;

    // Draw pulsing circle with radial gradient using single atomic blit
    int32_t center_x = rel_main_->relativeToAbsoluteX(x);
    int32_t center_y = rel_main_->relativeToAbsoluteY(y);
    int32_t radius_px = static_cast<int32_t>((radius / 100.0f) * ((width_ + height_) / 2.0f));
    if (radius_px < 1) radius_px = 1;  // Ensure at least 1 pixel

    // Calculate bounding box that covers BOTH old and new indicator positions
    // This ensures we erase the old indicator when drawing the new one
    int32_t old_left = has_drawn_indicator_ ? (last_indicator_x_ - last_indicator_radius_ - 1) : center_x;
    int32_t old_right = has_drawn_indicator_ ? (last_indicator_x_ + last_indicator_radius_ + 1) : center_x;
    int32_t old_top = has_drawn_indicator_ ? (last_indicator_y_ - last_indicator_radius_ - 1) : center_y;
    int32_t old_bottom = has_drawn_indicator_ ? (last_indicator_y_ + last_indicator_radius_ + 1) : center_y;

    int32_t new_left = center_x - radius_px - 1;
    int32_t new_right = center_x + radius_px + 1;
    int32_t new_top = center_y - radius_px - 1;
    int32_t new_bottom = center_y + radius_px + 1;

    // Union of both bounding boxes
    int32_t box_x = (old_left < new_left) ? old_left : new_left;
    int32_t box_y = (old_top < new_top) ? old_top : new_top;
    int32_t box_right = (old_right > new_right) ? old_right : new_right;
    int32_t box_bottom = (old_bottom > new_bottom) ? old_bottom : new_bottom;

    // Clamp to screen bounds
    if (box_x < 0) box_x = 0;
    if (box_y < 0) box_y = 0;
    if (box_right >= width_) box_right = width_ - 1;
    if (box_bottom >= height_) box_bottom = height_ - 1;

    int32_t box_width = box_right - box_x + 1;
    int32_t box_height = box_bottom - box_y + 1;

    if (box_width <= 0 || box_height <= 0 || composite_buffer_ == nullptr) return;

    // Allocate temp buffer for the region
    size_t buffer_size = box_width * box_height;
    uint16_t* region_buffer = static_cast<uint16_t*>(malloc(buffer_size * sizeof(uint16_t)));
    if (region_buffer == nullptr) return;

    // Step 1: Copy background from composite buffer (this erases old indicator)
    for (int32_t row = 0; row < box_height; row++) {
        int32_t src_y = box_y + row;
        size_t src_offset = static_cast<size_t>(src_y) * static_cast<size_t>(width_) + static_cast<size_t>(box_x);
        size_t dst_offset = static_cast<size_t>(row) * static_cast<size_t>(box_width);
        memcpy(&region_buffer[dst_offset], &composite_buffer_[src_offset], box_width * sizeof(uint16_t));
    }

    // Step 2: Render new indicator into the temp buffer
    for (int32_t py = box_y; py <= box_bottom; py++) {
        for (int32_t px = box_x; px <= box_right; px++) {
            int32_t dx = px - center_x;
            int32_t dy = py - center_y;
            float dist = sqrtf(static_cast<float>(dx * dx + dy * dy));

            if (dist <= static_cast<float>(radius_px)) {
                float t = (radius_px > 0) ? (dist / static_cast<float>(radius_px)) : 0.0f;
                uint16_t color = interpolate_color(
                    theme_.liveIndicatorGradient.color_stops[0],
                    theme_.liveIndicatorGradient.color_stops[1],
                    t
                );

                // Write to buffer
                int32_t buffer_x = px - box_x;
                int32_t buffer_y = py - box_y;
                size_t buffer_index = static_cast<size_t>(buffer_y) * static_cast<size_t>(box_width) + static_cast<size_t>(buffer_x);
                region_buffer[buffer_index] = color;
            }
        }
    }

    // Step 3: Single atomic blit to display (erase + draw in one operation)
    hal_display_fast_blit(static_cast<int16_t>(box_x), static_cast<int16_t>(box_y),
                          static_cast<int16_t>(box_width), static_cast<int16_t>(box_height),
                          region_buffer);

    free(region_buffer);

    // Track indicator position for next frame
    last_indicator_x_ = center_x;
    last_indicator_y_ = center_y;
    last_indicator_radius_ = radius_px;
    has_drawn_indicator_ = true;
}

float TimeSeriesGraph::mapYToScreen(double y_value, double y_min, double y_max) {
    float y_range = static_cast<float>(y_max - y_min);
    float normalized = static_cast<float>(y_value - y_min) / y_range;

    GraphMargins m = getMargins();
    float screen_y_min = m.top;
    float screen_y_max = 100.0f - m.bottom;
    float screen_range = screen_y_max - screen_y_min;

    // Invert Y-axis (higher values at top)
    return screen_y_max - (normalized * screen_range);
}

float TimeSeriesGraph::mapXToScreen(size_t x_index, size_t x_count) {
    float normalized = static_cast<float>(x_index) / static_cast<float>(x_count - 1);

    GraphMargins m = getMargins();
    float screen_x_min = m.left;
    float screen_x_max = 100.0f - m.right;
    float screen_range = screen_x_max - screen_x_min;

    return screen_x_min + (normalized * screen_range);
}
