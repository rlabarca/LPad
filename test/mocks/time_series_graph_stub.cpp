/**
 * @file time_series_graph_stub.cpp
 * @brief Native-test stub for TimeSeriesGraph
 *
 * Provides no-op implementations of all TimeSeriesGraph public methods so that
 * app classes (StockTickerApp) that create TimeSeriesGraph instances can be
 * compiled and unit-tested on the native platform without PSRAM or GFX hardware.
 *
 * Used only when UNIT_TEST is defined (native_test environment).
 */

#include "ui_time_series_graph.h"
#include <cstdio>
#include <cstring>

TimeSeriesGraph::TimeSeriesGraph(const GraphTheme& theme, Arduino_GFX* main_display,
                                 int32_t width, int32_t height)
    : theme_(theme)
    , data_()
    , width_(width)
    , height_(height)
    , main_display_(main_display)
    , bg_canvas_(nullptr)
    , data_canvas_(nullptr)
    , rel_main_(nullptr)
    , rel_bg_(nullptr)
    , rel_data_(nullptr)
    , composite_buffer_(nullptr)
    , composite_buffer_size_(0)
    , pulse_phase_(0.0f)
    , y_tick_increment_(0.0f)
    , tick_label_position_(TickLabelPosition::INSIDE)
    , x_axis_title_(nullptr)
    , y_axis_title_(nullptr)
    , watermarkText_(nullptr)
    , last_indicator_x_(0)
    , last_indicator_y_(0)
    , last_indicator_radius_(0)
    , has_drawn_indicator_(false)
    , cached_y_min_(0.0)
    , cached_y_max_(0.0)
    , range_cached_(false)
{}

TimeSeriesGraph::~TimeSeriesGraph() {}

bool TimeSeriesGraph::begin() { return true; }

void TimeSeriesGraph::setData(const GraphData& data) { data_ = data; }

void TimeSeriesGraph::setYTicks(float increment) { y_tick_increment_ = increment; }

void TimeSeriesGraph::setTickLabelPosition(TickLabelPosition pos) {
    tick_label_position_ = pos;
}

void TimeSeriesGraph::setXAxisTitle(const char* title) { x_axis_title_ = title; }

void TimeSeriesGraph::setYAxisTitle(const char* title) { y_axis_title_ = title; }

void TimeSeriesGraph::setWatermark(const char* text) { watermarkText_ = text; }

void TimeSeriesGraph::setTheme(const GraphTheme& theme) { theme_ = theme; }

void TimeSeriesGraph::drawBackground() {}

void TimeSeriesGraph::drawData() {}

void TimeSeriesGraph::render() {}

void TimeSeriesGraph::update(float deltaTime) { (void)deltaTime; }

float TimeSeriesGraph::mapYToScreen(double y_value, double y_min, double y_max) {
    (void)y_value; (void)y_min; (void)y_max;
    return 0.0f;
}

float TimeSeriesGraph::mapXToScreen(size_t x_index, size_t x_count) {
    (void)x_index; (void)x_count;
    return 0.0f;
}

TimeSeriesGraph::GraphMargins TimeSeriesGraph::getMargins() const {
    return {0.0f, 0.0f, 0.0f, 0.0f};
}

void TimeSeriesGraph::formatValue(double value, char* buffer, size_t buffer_size) {
    (void)value;
    if (buffer && buffer_size > 0) {
        std::snprintf(buffer, buffer_size, "0.00");
    }
}
