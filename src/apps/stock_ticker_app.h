/**
 * @file stock_ticker_app.h
 * @brief Standalone Stock Ticker Application Component (Z=1)
 *
 * Directly owns StockTracker and TimeSeriesGraph — no V060DemoApp wrapper.
 * Registered as an AppComponent with the UIRenderManager.
 */

#ifndef STOCK_TICKER_APP_H
#define STOCK_TICKER_APP_H

#include "../ui/ui_component.h"

class RelativeDisplay;
class TimeSeriesGraph;
class StockTracker;
struct GraphTheme;

class StockTickerApp : public AppComponent {
public:
    StockTickerApp();
    ~StockTickerApp();

    bool begin(RelativeDisplay* display);

    // UIComponent lifecycle
    void onRun() override;
    void onPause() override {}
    void onUnpause() override;
    void onClose() override;
    void render() override;
    void update(float dt) override;
    bool handleInput(const touch_gesture_event_t& event) override;

    bool isOpaque() const override { return true; }
    bool isFullscreen() const override { return true; }

private:
    RelativeDisplay* m_display;
    TimeSeriesGraph* m_graph;
    StockTracker* m_stockTracker;

    bool m_backgroundDrawn;
    bool m_graphInitialRenderDone;
    long m_lastDataTimestamp;

    GraphTheme createStockGraphTheme();
};

#endif // STOCK_TICKER_APP_H
