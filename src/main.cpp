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
#include "yahoo_chart_parser.h"
#include "animation_ticker.h"

// Custom RGB565 colors for the demo
#define RGB565_DARK_PURPLE 0x4810  // Deep purple
#define RGB565_DARK_BLUE   0x001F  // Pure deep blue

// Embedded test data from test_data/yahoo_chart_tnx_5m_1d.json
const char* TEST_DATA_JSON = R"({"chart":{"result":[{"meta":{"currency":"USD","symbol":"^TNX","exchangeName":"CGI","fullExchangeName":"Cboe Indices","instrumentType":"INDEX","firstTradeDate":-252326400,"regularMarketTime":1770062392,"hasPrePostMarketData":false,"gmtoffset":-21600,"timezone":"CST","exchangeTimezoneName":"America/Chicago","regularMarketPrice":4.275,"fiftyTwoWeekHigh":4.997,"fiftyTwoWeekLow":3.345,"regularMarketDayHigh":4.261,"regularMarketDayLow":4.237,"regularMarketVolume":0,"longName":"CBOE Interest Rate 10 Year T No","shortName":"CBOE Interest Rate 10 Year T No","chartPreviousClose":4.227,"previousClose":4.227,"scale":3,"priceHint":4,"currentTradingPeriod":{"pre":{"timezone":"CST","end":1770038400,"start":1770038400,"gmtoffset":-21600},"regular":{"timezone":"CST","end":1770062400,"start":1770038400,"gmtoffset":-21600},"post":{"timezone":"CST","end":1770062400,"start":1770062400,"gmtoffset":-21600}},"tradingPeriods":[[{"timezone":"CST","end":1770062400,"start":1770038400,"gmtoffset":-21600}]],"dataGranularity":"5m","range":"1d","validRanges":["1d","5d","1mo","3mo","6mo","1y","2y","5y","10y","ytd","max"]},"timestamp":[1770057900,1770058200,1770058500,1770058800,1770059100,1770059400,1770059700,1770060000,1770060300,1770060600,1770060900,1770061200,1770061500,1770061800,1770062100],"indicators":{"quote":[{"open":[4.270999908447266,4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.2769999504089355,4.275000095367432,4.2769999504089355,4.279000282287598,4.279000282287598,4.2769999504089355,4.279000282287598,4.275000095367432,4.2729997634887695,4.2729997634887695],"close":[4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.2769999504089355,4.275000095367432,4.2769999504089355,4.279000282287598,4.279000282287598,4.2769999504089355,4.2769999504089355,4.275000095367432,4.2729997634887695,4.2729997634887695,4.275000095367432],"high":[4.2729997634887695,4.2729997634887695,4.275000095367432,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.279000282287598,4.279000282287598,4.279000282287598,4.279000282287598,4.279000282287598,4.275000095367432,4.2729997634887695,4.275000095367432],"volume":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"low":[4.270999908447266,4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.275000095367432,4.275000095367432,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.275000095367432,4.2729997634887695,4.2729997634887695,4.269000053405762]}]}}],"error":null}})";

// Global variables
TimeSeriesGraph* g_graph = nullptr;
AnimationTicker* g_ticker = nullptr;
GraphData g_graphData;  // Store data for graph

// Create Vaporwave theme with integrated live indicator
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

    // Enable integrated live indicator with dirty-rect optimization
    // Note: Using integrated indicator for flicker-free animation
    theme.liveIndicatorGradient.color_stops[0] = RGB565_MAGENTA;  // Pink/magenta center
    theme.liveIndicatorGradient.color_stops[1] = RGB565_CYAN;     // Cyan edge
    theme.liveIndicatorPulseSpeed = 0.5f;  // 0.5 pulses per second (2 second cycle)

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

    // [2/6] Initialize RelativeDisplay API
    Serial.println("[2/6] Initializing RelativeDisplay abstraction...");
    display_relative_init();
    Arduino_GFX* display = static_cast<Arduino_GFX*>(hal_display_get_gfx());
    if (display == nullptr) {
        displayError("Display object unavailable");
        while (1) delay(1000);
    }
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

    // TimeSeriesGraph with layered rendering and integrated live indicator
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
    Serial.println("  [PASS] TimeSeriesGraph created with integrated indicator");
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

    if (g_graph == nullptr) {
        return;
    }

    // Update the graph with integrated live indicator animation
    // This uses dirty-rect optimization to avoid flashing
    g_graph->update(deltaTime);

    // Flush to ensure display updates
    hal_display_flush();
}
