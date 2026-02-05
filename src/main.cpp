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
#include <algorithm>

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
GraphData g_graphData;  // Store data to calculate indicator position

// Create Vaporwave theme with standalone background
GraphTheme createVaporwaveTheme() {
    GraphTheme theme = {};

    // Basic colors
    theme.backgroundColor = RGB565_DARK_PURPLE;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_MAGENTA;

    // Enable 45-degree background gradient (Purple -> Pink -> Dark Blue)
    theme.useBackgroundGradient = true;
    theme.backgroundGradient.angle_deg = 45.0f;
    theme.backgroundGradient.color_stops[0] = RGB565_DARK_PURPLE;
    theme.backgroundGradient.color_stops[1] = RGB565_MAGENTA;
    theme.backgroundGradient.color_stops[2] = RGB565_DARK_BLUE;
    theme.backgroundGradient.num_stops = 3;

    // Enable gradient line (horizontal, Cyan to Pink)
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

    // Disable integrated live indicator (using standalone component)
    theme.liveIndicatorGradient.color_stops[0] = 0;
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

    // [1/6] Initialize display HAL
    Serial.println("[1/6] Initializing display HAL...");
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

    // [2/6] Initialize RelativeDisplay
    Serial.println("[2/6] Initializing RelativeDisplay abstraction...");
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

    // [3/6] Create AnimationTicker
    Serial.println("[3/6] Creating 30fps AnimationTicker...");
    static AnimationTicker ticker(30);
    g_ticker = &ticker;
    Serial.println("  [PASS] AnimationTicker created (30fps)");
    Serial.println();
    delay(500);

    // [4/6] Parse test data
    Serial.println("[4/6] Parsing test data from embedded JSON...");
    YahooChartParser parser("");
    if (!parser.parseFromString(TEST_DATA_JSON)) {
        displayError("Failed to parse test data");
        while (1) delay(1000);
    }

    g_graphData.x_values = parser.getTimestamps();
    g_graphData.y_values = parser.getClosePrices();
    Serial.printf("  [PASS] Parsed %d data points\n", g_graphData.y_values.size());
    Serial.println();
    delay(500);

    // [5/6] Create UI components
    Serial.println("[5/6] Creating UI components...");

    // TimeSeriesGraph with layered rendering
    Serial.println("  Creating TimeSeriesGraph with Vaporwave theme...");
    GraphTheme theme = createVaporwaveTheme();
    static TimeSeriesGraph graph(theme, display, width, height);
    g_graph = &graph;

    if (!graph.begin()) {
        displayError("Graph initialization failed");
        while (1) delay(1000);
    }

    graph.setData(g_graphData);
    graph.setYTicks(0.002f);
    Serial.println("  [PASS] TimeSeriesGraph created");

    // LiveIndicator (standalone component)
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

    // [6/6] Initial render
    Serial.println("[6/6] Performing initial render...");

    // Draw static layers to canvases
    Serial.println("  Drawing background layer...");
    graph.drawBackground();

    Serial.println("  Drawing data layer...");
    graph.drawData();

    // Composite and render to display
    Serial.println("  Compositing to display...");
    graph.render();
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

    if (g_graph == nullptr || g_indicator == nullptr || g_graphData.y_values.empty()) {
        return;
    }

    // Update indicator animation
    g_indicator->update(deltaTime);

    // Calculate position of last data point
    size_t last_idx = g_graphData.y_values.size() - 1;

    // X position (maps to graph's drawing area: 10% to 90%)
    float x_percent = 10.0f + (80.0f * static_cast<float>(last_idx) /
                               static_cast<float>(g_graphData.y_values.size() - 1));

    // Y position (maps to graph's drawing area: 10% to 90%, inverted for screen coords)
    double y_min = *std::min_element(g_graphData.y_values.begin(), g_graphData.y_values.end());
    double y_max = *std::max_element(g_graphData.y_values.begin(), g_graphData.y_values.end());
    if (y_max - y_min < 0.001) y_max = y_min + 1.0;

    double y_value = g_graphData.y_values[last_idx];
    float y_normalized = static_cast<float>(y_value - y_min) / static_cast<float>(y_max - y_min);
    float y_percent = 10.0f + (80.0f * (1.0f - y_normalized));  // Inverted for screen coords

    // Draw indicator directly on top of the graph
    // Note: This draws directly to the display, on top of the already-rendered graph
    // For a production implementation, you'd use dirty-rect optimization
    g_indicator->draw(x_percent, y_percent);

    hal_display_flush();
}
