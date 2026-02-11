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
#include <string>
#include <vector>

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

// Helper function to format a number with 3 significant digits
static void format_3_sig_digits(double value, char* buffer, size_t buffer_size) {
    if (value == 0.0) {
        snprintf(buffer, buffer_size, "0.00");
        return;
    }

    // Calculate magnitude (power of 10)
    double abs_value = fabs(value);
    int magnitude = static_cast<int>(floor(log10(abs_value)));

    // Determine decimal places needed for 3 significant digits
    int decimal_places = 2 - magnitude;
    if (decimal_places < 0) {
        decimal_places = 0;
    } else if (decimal_places > 6) {
        decimal_places = 6;  // Reasonable upper limit
    }

    // Format the number
    snprintf(buffer, buffer_size, "%.*f", decimal_places, value);
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
    // Clear canvas to prevent corrupted frame flash from uninitialized PSRAM
    bg_canvas_->fillScreen(0x0000);
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
    // Clear canvas with chroma key to prevent corrupted frame flash
    constexpr uint16_t CHROMA_KEY = 0x0001;
    data_canvas_->fillScreen(CHROMA_KEY);
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
        m.top = 3.0f;
        m.right = 3.0f;
        // INSIDE mode: need more bottom margin for X-axis title
        // Text size 2 is ~14px tall, need room for title below axis line
        m.bottom = x_axis_title_ ? 12.0f : 3.0f;
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

    // Use built-in font at size 2 for axis titles (custom GFX fonts crash on PSRAM canvas)
    canvas->setFont(nullptr);
    canvas->setTextSize(2);
    canvas->setTextColor(theme_.tickColor);

    // X-axis title: centered horizontally, positioned to avoid axis line overlap
    if (x_axis_title_) {
        int16_t x1, y1;
        uint16_t w, h;
        canvas->getTextBounds(x_axis_title_, 0, 0, &x1, &y1, &w, &h);

        float graph_center_x = m.left + (100.0f - m.left - m.right) / 2.0f;
        int32_t center_px = target->relativeToAbsoluteX(graph_center_x);
        int32_t title_x = center_px - static_cast<int32_t>(w) / 2;

        // Position title based on label mode to ensure it doesn't overlap axis line
        int32_t title_y;
        if (tick_label_position_ == TickLabelPosition::OUTSIDE) {
            // OUTSIDE mode: Position in bottom margin below axis line
            float x_axis_y = 100.0f - m.bottom;
            int32_t x_axis_y_px = target->relativeToAbsoluteY(x_axis_y);
            title_y = x_axis_y_px + 6;  // 6 pixels below axis line
        } else {
            // INSIDE mode: Position below axis line with proper spacing
            // Axis is at (100% - bottom_margin), title goes below it
            float x_axis_y = 100.0f - m.bottom;
            int32_t x_axis_y_px = target->relativeToAbsoluteY(x_axis_y);
            title_y = x_axis_y_px + h + 4;  // Position below axis with 4px gap
        }

        if (title_x < 0) title_x = 0;
        if (title_x + static_cast<int32_t>(w) >= width_) title_x = width_ - w - 1;
        if (title_y < 0) title_y = 0;
        if (title_y + static_cast<int32_t>(h) >= height_) title_y = height_ - h - 1;

        canvas->setCursor(title_x, title_y);
        canvas->print(x_axis_title_);
    }

    // Y-axis title: character by character, vertically centered
    if (y_axis_title_) {
        int len = static_cast<int>(strlen(y_axis_title_));
        if (len == 0 || len > 32) return;

        // Built-in font at size 2: each char is 10w x 14h (with spacing 12x16)
        int32_t char_h = 14;
        int32_t char_w = 10;
        int32_t char_spacing = char_h + 2;
        int32_t total_height = len * char_spacing;

        // Center vertically in graph area
        float graph_center_y = (m.top + (100.0f - m.bottom)) / 2.0f;
        int32_t center_py = target->relativeToAbsoluteY(graph_center_y);
        int32_t start_y = center_py - total_height / 2;
        int32_t title_x = 2;

        for (int i = 0; i < len; i++) {
            int32_t char_y = start_y + i * char_spacing;

            if (char_y >= 0 && char_y + char_h < height_ && title_x >= 0) {
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

    Serial.printf("[Graph] Y-axis range: min=%.6f, max=%.6f, range=%.6f, tick_increment=%.6f\n",
                  y_min, y_max, y_max - y_min, y_tick_increment_);

    if (y_max - y_min < 0.001) return;

    GraphMargins m = getMargins();
    float x_axis = m.left;

    Arduino_GFX* canvas = target->getGfx();
    if (!canvas) return;

    // Use built-in font at size 2 for better visibility
    // (custom GFX fonts crash on PSRAM canvas)
    canvas->setFont(nullptr);
    canvas->setTextColor(theme_.tickColor);
    canvas->setTextSize(2);

    // Draw tick marks and labels (skip first tick at y_min to avoid X-axis overlap)
    // Origin suppression: skip the very first tick that's too close to X-axis
    GraphMargins mx = getMargins();
    float x_axis_y = 100.0f - mx.bottom;  // X-axis position in relative coordinates

    // Track seen labels to enforce unique label constraint
    // When data range is small relative to tick spacing, labels may duplicate
    // Strategy: skip every Nth tick until labels are unique (adjust tick density)
    std::vector<std::string> seen_labels;
    int tick_skip = 1;  // Show every tick initially

    // First pass: generate "clean" tick values
    // Per spec: ticks must be at exact clean values (e.g., 4.19000, not 4.18732 rounded to 4.19)
    // Strategy: Use integer multiples of increment to avoid floating-point accumulation errors

    // Round y_min UP to next clean multiple of tick increment
    double first_tick = ceil(y_min / y_tick_increment_) * y_tick_increment_;

    // Calculate how many ticks to generate
    int num_ticks = static_cast<int>((y_max - first_tick) / y_tick_increment_) + 1;

    // Generate clean tick values using integer multiples (avoids floating-point drift)
    std::vector<std::pair<double, float>> all_ticks;  // Store tick_value and y_screen
    int suppressed_count = 0;

    for (int i = 0; i < num_ticks; i++) {
        // Use integer multiple to ensure exact clean values
        double tick_value = first_tick + (i * y_tick_increment_);

        // Round to remove floating-point garbage bits
        // Determine precision needed based on increment magnitude
        // For increment 0.002, need at least 3 decimal places
        double rounding_factor = 1.0 / y_tick_increment_;
        tick_value = round(tick_value * rounding_factor) / rounding_factor;

        // Don't exceed y_max
        if (tick_value > y_max + 1e-9) break;

        // Map this clean value to screen position
        float y_screen = mapYToScreen(tick_value, y_min, y_max);

        // Origin suppression: skip ticks too close to X-axis
        if (fabsf(y_screen - x_axis_y) < 8.0f) {
            suppressed_count++;
            continue;
        }

        all_ticks.push_back({tick_value, y_screen});
    }

    Serial.printf("[Graph] Generated %zu Y-ticks (suppressed %d near X-axis)\n", all_ticks.size(), suppressed_count);
    Serial.printf("[Graph] Clean tick values: first=%.9f, increment=%.9f\n", first_tick, y_tick_increment_);

    // Debug: log actual tick values and screen positions to verify uniform spacing
    if (all_ticks.size() > 0) {
        Serial.println("[Graph] Tick debug (value, screen_y, delta):");
        for (size_t i = 0; i < all_ticks.size(); i++) {
            float delta = (i > 0) ? (all_ticks[i].second - all_ticks[i-1].second) : 0.0f;
            Serial.printf("  [%zu] value=%.9f, screen_y=%.3f, delta=%.3f\n",
                          i, all_ticks[i].first, all_ticks[i].second, delta);
        }
    }

    // Check for duplicates in formatted labels
    std::vector<std::string> test_labels;
    for (const auto& tick : all_ticks) {
        char label[16];
        format_3_sig_digits(tick.first, label, sizeof(label));
        test_labels.push_back(std::string(label));
    }

    // If duplicates found, increase tick skip factor to reduce density
    bool found_duplicates = false;
    for (size_t i = 0; i < test_labels.size(); i++) {
        for (size_t j = i + 1; j < test_labels.size(); j++) {
            if (test_labels[i] == test_labels[j]) {
                tick_skip = 2;  // Show every 2nd tick
                found_duplicates = true;
                break;
            }
        }
        if (tick_skip > 1) break;
    }
    if (found_duplicates) {
        Serial.printf("[Graph] Duplicate labels detected, tick_skip increased to %d\n", tick_skip);
    }

    // Second pass: render ticks with adjusted density
    int tick_index = 0;
    for (const auto& tick : all_ticks) {
        // Skip ticks based on density adjustment
        if (tick_index % tick_skip != 0) {
            tick_index++;
            continue;
        }
        tick_index++;

        double tick_value = tick.first;
        float y_screen = tick.second;

        // Format label
        char label[16];
        format_3_sig_digits(tick_value, label, sizeof(label));

        // Check for duplicates (should be rare now with skip factor)
        std::string label_str(label);
        bool is_duplicate = false;
        for (const auto& seen : seen_labels) {
            if (seen == label_str) {
                is_duplicate = true;
                break;
            }
        }
        if (is_duplicate) continue;
        seen_labels.push_back(label_str);

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
            // Vertically center label on tick mark
            // getTextBounds returns y1 (offset from baseline to top, usually negative)
            // Text center is at baseline + y1 + h/2
            // To center at label_y: baseline = label_y - y1 - h/2
            canvas->setCursor(label_x, label_y - y1 - h / 2);
            canvas->print(label);
        } else {
            // INSIDE: tick extends RIGHT into graph
            float tick_end = x_axis + theme_.tickLength;
            target->drawHorizontalLine(y_screen, x_axis, tick_end, theme_.tickColor);

            // Label to RIGHT of tick (inside graph), leaving small gap
            int32_t label_x = target->relativeToAbsoluteX(tick_end) + 2;  // 2px gap after tick
            int32_t label_y = target->relativeToAbsoluteY(y_screen);
            // Vertically center label on tick mark
            // getTextBounds returns y1 (offset from baseline to top, usually negative)
            // Text center is at baseline + y1 + h/2
            // To center at label_y: baseline = label_y - y1 - h/2
            canvas->setCursor(label_x, label_y - y1 - h / 2);
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

    // Use built-in font at size 2 for better visibility
    // (custom GFX fonts crash on PSRAM canvas)
    canvas->setFont(nullptr);
    canvas->setTextColor(theme_.tickColor);
    canvas->setTextSize(2);

    size_t num_points = data_.x_values.size();
    if (num_points < 2) return;

    // Get the latest timestamp (last data point)
    long latest_timestamp = data_.x_values[num_points - 1];

    size_t tick_interval = (num_points > 5) ? (num_points / 5) : 1;

    // Track previous label to skip duplicates (happens when data points are very close in time)
    long prev_hours_prior = -999;  // Initialize to impossible value

    // Skip first tick near Y-axis
    for (size_t i = tick_interval; i < num_points; i += tick_interval) {
        float x_screen = mapXToScreen(i, num_points);

        char label[16];
        long hours_prior = 0;
        if (i < data_.x_values.size()) {
            long timestamp = data_.x_values[i];
            // Calculate hours prior to latest data point
            long seconds_prior = latest_timestamp - timestamp;
            hours_prior = seconds_prior / 3600;

            // Skip this tick if it has the same hours_prior as previous tick
            // (happens when data points within same hour)
            if (hours_prior == prev_hours_prior) {
                continue;
            }
            prev_hours_prior = hours_prior;

            snprintf(label, sizeof(label), "%ld", hours_prior);
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
            // Position label so its BOTTOM is above tick_top, not overlapping
            int32_t label_x = target->relativeToAbsoluteX(x_screen) - w / 2;
            int32_t tick_top_px = target->relativeToAbsoluteY(tick_top);
            // setCursor sets baseline, text extends ~h above baseline for built-in fonts
            // Position baseline so text bottom (baseline - h/4) is 2px above tick_top
            int32_t label_y = tick_top_px - h + 2;
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

    // If data range is very small (all values nearly identical), center them vertically
    // instead of clamping to bottom. This handles initial data where all points may have
    // the same value, making the line appear in the middle of the graph.
    if (y_max - y_min < 0.001) {
        double center = y_min;
        y_min = center - 0.5;
        y_max = center + 0.5;
    }

    size_t point_count = data_.y_values.size();

    // Calculate line thickness in pixels (reduced by 20% for visual refinement)
    float thickness_pct = theme_.lineThickness * 0.80f;  // Reduce by 20%
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
    float max_radius = 3.0f;  // Maximum radius in relative % (half of previous 6.0)

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
