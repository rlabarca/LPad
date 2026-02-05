/**
 * @file main.cpp
 * @brief Base UI Demo Application
 *
 * This application demonstrates the full capabilities of the LPad UI system:
 * - HAL abstraction for hardware independence
 * - Resolution-independent display via RelativeDisplay
 * - Layered rendering with off-screen canvases
 * - Smooth 30fps animation via AnimationTicker
 * - Gradient backgrounds
 * - Time series graphs
 * - Animated live indicators
 *
 * See features/app_demo_screen.md for specification.
 */

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "../hal/display.h"
#include "relative_display.h"
#include "ui_time_series_graph.h"
#include "ui_live_indicator.h"
#include "yahoo_chart_parser.h"
#include "animation_ticker.h"

// Custom RGB565 colors for the demo
#define RGB565_DARK_PURPLE 0x4810  // Deep purple
#define RGB565_DARK_BLUE   0x001F  // Pure deep blue

// Embedded test data from test_data/yahoo_chart_tnx_5m_1d.json
const char* TEST_DATA_JSON = R"({"chart":{"result":[{"meta":{"currency":"USD","symbol":"^TNX","exchangeName":"CGI","fullExchangeName":"Cboe Indices","instrumentType":"INDEX","firstTradeDate":-252326400,"regularMarketTime":1770062392,"hasPrePostMarketData":false,"gmtoffset":-21600,"timezone":"CST","exchangeTimezoneName":"America/Chicago","regularMarketPrice":4.275,"fiftyTwoWeekHigh":4.997,"fiftyTwoWeekLow":3.345,"regularMarketDayHigh":4.261,"regularMarketDayLow":4.237,"regularMarketVolume":0,"longName":"CBOE Interest Rate 10 Year T No","shortName":"CBOE Interest Rate 10 Year T No","chartPreviousClose":4.227,"previousClose":4.227,"scale":3,"priceHint":4,"currentTradingPeriod":{"pre":{"timezone":"CST","end":1770038400,"start":1770038400,"gmtoffset":-21600},"regular":{"timezone":"CST","end":1770062400,"start":1770038400,"gmtoffset":-21600},"post":{"timezone":"CST","end":1770062400,"start":1770062400,"gmtoffset":-21600}},"tradingPeriods":[[{"timezone":"CST","end":1770062400,"start":1770038400,"gmtoffset":-21600}]],"dataGranularity":"5m","range":"1d","validRanges":["1d","5d","1mo","3mo","6mo","1y","2y","5y","10y","ytd","max"]},"timestamp":[1770057900,1770058200,1770058500,1770058800,1770059100,1770059400,1770059700,1770060000,1770060300,1770060600,1770060900,1770061200,1770061500,1770061800,1770062100],"indicators":{"quote":[{"open":[4.270999908447266,4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.2769999504089355,4.275000095367432,4.2769999504089355,4.279000282287598,4.279000282287598,4.2769999504089355,4.279000282287598,4.275000095367432,4.2729997634887695,4.2729997634887695],"close":[4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.2769999504089355,4.275000095367432,4.2769999504089355,4.279000282287598,4.279000282287598,4.2769999504089355,4.2769999504089355,4.275000095367432,4.2729997634887695,4.2729997634887695,4.275000095367432],"high":[4.2729997634887695,4.2729997634887695,4.275000095367432,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.279000282287598,4.279000282287598,4.279000282287598,4.279000282287598,4.279000282287598,4.275000095367432,4.2729997634887695,4.275000095367432],"volume":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"low":[4.270999908447266,4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.275000095367432,4.275000095367432,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.275000095367432,4.2729997634887695,4.2729997634887695,4.269000053405762]}]}}],"error":null}})";

// Global variables
TimeSeriesGraph* g_graph = nullptr;
LiveIndicator* g_indicator = nullptr;
AnimationTicker* g_ticker = nullptr;
RelativeDisplay* g_relDisplay = nullptr;
Arduino_Canvas* g_main_canvas = nullptr;

// Create Vaporwave theme for the graph
GraphTheme createVaporwaveTheme() {
    GraphTheme theme = {};

    // Basic colors (no background gradient in graph itself)
    theme.backgroundColor = RGB565_DARK_PURPLE;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_MAGENTA;

    // Graph line with horizontal gradient (Cyan to Pink)
    theme.useLineGradient = true;
    theme.lineGradient.angle_deg = 0.0f;
    theme.lineGradient.color_stops[0] = RGB565_CYAN;
    theme.lineGradient.color_stops[1] = RGB565_MAGENTA;
    theme.lineGradient.num_stops = 2;

    // Line and axis styling
    theme.lineThickness = 2.0f;
    theme.axisThickness = 0.8f;
    theme.tickColor = RGB565_WHITE;
    theme.tickLength = 2.5f;

    // Disable integrated background and indicator (we'll use standalone components)
    theme.useBackgroundGradient = false;
    theme.liveIndicatorGradient.color_stops[0] = 0;  // Not used
    theme.liveIndicatorGradient.color_stops[1] = 0;
    theme.liveIndicatorPulseSpeed = 0.0f;

    return theme;
}

void displayError(const char* message) {
    hal_display_clear(RGB565_RED);
    hal_display_flush();
    Serial.println("=== ERROR ===");
    Serial.println(message);
    Serial.println("=============");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== LPad Base UI Demo Application ===");
    Serial.println();

    // [1/7] Initialize display HAL
    Serial.println("[1/7] Initializing display HAL...");
    if (!hal_display_init()) {
        displayError("Display initialization failed");
        while (1) delay(1000);
    }
    Serial.println("  [PASS] Display initialized");

#ifdef APP_DISPLAY_ROTATION
    Serial.printf("  [INFO] Applying rotation: %d degrees\n", APP_DISPLAY_ROTATION);
    hal_display_set_rotation(APP_DISPLAY_ROTATION);
#endif

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();
    Serial.printf("  [INFO] Display resolution: %d x %d pixels\n", width, height);
    Serial.println();
    delay(500);

    // [2/7] Initialize RelativeDisplay
    Serial.println("[2/7] Initializing RelativeDisplay abstraction...");
    display_relative_init();
    Arduino_GFX* display = static_cast<Arduino_GFX*>(hal_display_get_gfx());
    if (display == nullptr) {
        displayError("Display object unavailable");
        while (1) delay(1000);
    }
    static RelativeDisplay relDisplay(display, width, height);
    g_relDisplay = &relDisplay;
    Serial.println("  [PASS] RelativeDisplay initialized");
    Serial.println();
    delay(500);

    // [3/7] Create AnimationTicker
    Serial.println("[3/7] Creating 30fps AnimationTicker...");
    static AnimationTicker ticker(30);
    g_ticker = &ticker;
    Serial.println("  [PASS] AnimationTicker created (30fps)");
    Serial.println();
    delay(500);

    // [4/7] Create main canvas
    Serial.println("[4/7] Creating off-screen main canvas...");
    Serial.printf("  Canvas size: %d x %d pixels\n", width, height);
    static Arduino_Canvas canvas(width, height, display, 0, 0, true);  // Use PSRAM
    g_main_canvas = &canvas;
    if (!canvas.begin()) {
        Serial.println("  [WARN] PSRAM allocation may have failed");
    }
    Serial.println("  [PASS] Main canvas created");
    Serial.println();
    delay(500);

    // [5/7] Parse test data
    Serial.println("[5/7] Parsing test data from embedded JSON...");
    YahooChartParser parser("");
    if (!parser.parseFromString(TEST_DATA_JSON)) {
        displayError("Failed to parse test data");
        while (1) delay(1000);
    }

    const std::vector<long>& timestamps = parser.getTimestamps();
    const std::vector<double>& closePrices = parser.getClosePrices();
    Serial.printf("  [PASS] Parsed %d data points\n", closePrices.size());
    Serial.println();
    delay(500);

    // [6/7] Create UI components
    Serial.println("[6/7] Creating UI components...");

    // TimeSeriesGraph
    Serial.println("  Creating TimeSeriesGraph with Vaporwave theme...");
    GraphTheme theme = createVaporwaveTheme();
    static TimeSeriesGraph graph(theme, g_main_canvas, width, height);
    g_graph = &graph;

    GraphData graphData;
    graphData.x_values = timestamps;
    graphData.y_values = closePrices;
    graph.setData(graphData);
    graph.setYTicks(0.002f);

    Serial.println("  [PASS] TimeSeriesGraph created");

    // LiveIndicator
    Serial.println("  Creating LiveIndicator component...");
    IndicatorTheme indicatorTheme;
    indicatorTheme.innerColor = RGB565_MAGENTA;  // Pink/magenta center
    indicatorTheme.outerColor = RGB565_CYAN;     // Cyan edge
    indicatorTheme.minRadius = 1.0f;
    indicatorTheme.maxRadius = 6.0f;
    indicatorTheme.pulseDuration = 2000.0f;  // 2 second pulse cycle

    static LiveIndicator indicator(indicatorTheme, g_relDisplay);
    g_indicator = &indicator;
    Serial.println("  [PASS] LiveIndicator created");
    Serial.println();
    delay(500);

    // [7/7] Initial render
    Serial.println("[7/7] Performing initial render...");

    // Select main canvas as drawing target
    hal_display_select_canvas(g_main_canvas);

    // Draw 45-degree gradient background (Purple -> Pink -> Dark Blue)
    Serial.println("  Drawing gradient background...");
    g_relDisplay->drawGradientBackground(
        RGB565_DARK_PURPLE,  // Top-left: Deep purple
        RGB565_MAGENTA,      // Middle: Bright magenta/pink
        RGB565_DARK_BLUE,    // Bottom-right: Pure deep blue
        45.0f                // 45-degree diagonal
    );

    // Draw graph (axes, ticks, data line)
    Serial.println("  Drawing time series graph...");
    if (!graph.begin()) {
        displayError("Graph initialization failed");
        while (1) delay(1000);
    }
    graph.drawBackground();
    graph.drawData();

    // Note: LiveIndicator will be drawn in the animation loop

    // Blit main canvas to display
    Serial.println("  Blitting main canvas to display...");
    hal_display_select_canvas(nullptr);  // Select main display
    hal_display_fast_blit(0, 0, width, height,
        static_cast<uint16_t*>(g_main_canvas->getFramebuffer()));
    hal_display_flush();

    Serial.println("  [PASS] Initial render complete");
    Serial.println();

    Serial.println("=== Demo Application Ready ===");
    Serial.println("Visual Verification:");
    Serial.println("  [ ] 45-degree gradient background (purple->pink->blue)");
    Serial.println("  [ ] Time series graph with gradient line (cyan->pink)");
    Serial.println("  [ ] Magenta axes with white tick marks");
    Serial.println("  [ ] Pulsing live indicator at last data point (30fps)");
    Serial.println();
    Serial.println("Starting 30fps animation loop...");
    Serial.println();
}

void loop() {
    // Wait for next frame and get deltaTime
    float deltaTime = g_ticker->waitForNextFrame();

    if (g_graph == nullptr || g_indicator == nullptr || g_relDisplay == nullptr) {
        return;
    }

    // Update indicator animation
    g_indicator->update(deltaTime);

    // TODO: Get last data point position from graph
    // For now, manually calculate the last point position
    // This should match the graph's mapXToScreen and mapYToScreen logic

    // Draw updated indicator directly to display
    // (The background+graph composite is already on the display from setup)

    // For simplicity in this demo, we'll redraw the entire frame
    // In a production app, you'd use dirty rect optimization

    // Select main canvas
    hal_display_select_canvas(g_main_canvas);

    // Redraw background
    g_relDisplay->drawGradientBackground(
        RGB565_DARK_PURPLE,
        RGB565_MAGENTA,
        RGB565_DARK_BLUE,
        45.0f
    );

    // Redraw graph
    g_graph->drawBackground();
    g_graph->drawData();

    // Draw indicator at last data point
    // Calculate position (this mirrors TimeSeriesGraph's calculation)
    const GraphData& data = g_graph->getData();
    if (!data.y_values.empty()) {
        size_t last_idx = data.y_values.size() - 1;

        // Simple position calculation (matches graph's logic)
        float x_percent = 10.0f + (80.0f * static_cast<float>(last_idx) /
                                   static_cast<float>(data.y_values.size() - 1));

        // Y position needs to map data value to screen space
        double y_min = *std::min_element(data.y_values.begin(), data.y_values.end());
        double y_max = *std::max_element(data.y_values.begin(), data.y_values.end());
        if (y_max - y_min < 0.001) y_max = y_min + 1.0;

        double y_value = data.y_values[last_idx];
        float y_normalized = static_cast<float>(y_value - y_min) /
                            static_cast<float>(y_max - y_min);
        float y_percent = 10.0f + (80.0f * (1.0f - y_normalized));  // Inverted for screen coords

        g_indicator->draw(x_percent, y_percent);
    }

    // Blit to display
    hal_display_select_canvas(nullptr);
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();
    hal_display_fast_blit(0, 0, width, height,
        static_cast<uint16_t*>(g_main_canvas->getFramebuffer()));
    hal_display_flush();
}
