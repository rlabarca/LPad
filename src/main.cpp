/**
 * @file main.cpp
 * @brief 10-Year Treasury Bond Tracker Application
 *
 * This application displays real-time 10-year treasury bond yield data
 * by orchestrating the YahooChartParser and TimeSeriesGraph components.
 *
 * Features:
 * - Parses Yahoo Chart API JSON data for ^TNX (10-year treasury)
 * - Renders time-series graph with vaporwave aesthetic
 * - Resolution-independent display via RelativeDisplay abstraction
 * - Graceful error handling for missing/invalid data
 *
 * See features/app_bond_tracker.md for specification.
 */

#include <Arduino.h>
#include "../hal/display.h"
#include "relative_display.h"
#include "ui_time_series_graph.h"
#include "yahoo_chart_parser.h"

// RGB565 color definitions
#define RGB565_BLACK       0x0000
#define RGB565_WHITE       0xFFFF
#define RGB565_CYAN        0x07FF
#define RGB565_MAGENTA     0xF81F
#define RGB565_DARK_PURPLE 0x4810
#define RGB565_RED         0xF800

// Embedded test data for ESP32 (since filesystem is not configured)
// This is the same data from test_data/yahoo_chart_tnx_5m_1d.json
const char* BOND_DATA_JSON = R"({"chart":{"result":[{"meta":{"currency":"USD","symbol":"^TNX","exchangeName":"CGI","fullExchangeName":"Cboe Indices","instrumentType":"INDEX","firstTradeDate":-252326400,"regularMarketTime":1770062392,"hasPrePostMarketData":false,"gmtoffset":-21600,"timezone":"CST","exchangeTimezoneName":"America/Chicago","regularMarketPrice":4.275,"fiftyTwoWeekHigh":4.997,"fiftyTwoWeekLow":3.345,"regularMarketDayHigh":4.261,"regularMarketDayLow":4.237,"regularMarketVolume":0,"longName":"CBOE Interest Rate 10 Year T No","shortName":"CBOE Interest Rate 10 Year T No","chartPreviousClose":4.227,"previousClose":4.227,"scale":3,"priceHint":4,"currentTradingPeriod":{"pre":{"timezone":"CST","end":1770038400,"start":1770038400,"gmtoffset":-21600},"regular":{"timezone":"CST","end":1770062400,"start":1770038400,"gmtoffset":-21600},"post":{"timezone":"CST","end":1770062400,"start":1770062400,"gmtoffset":-21600}},"tradingPeriods":[[{"timezone":"CST","end":1770062400,"start":1770038400,"gmtoffset":-21600}]],"dataGranularity":"5m","range":"1d","validRanges":["1d","5d","1mo","3mo","6mo","1y","2y","5y","10y","ytd","max"]},"timestamp":[1770057900,1770058200,1770058500,1770058800,1770059100,1770059400,1770059700,1770060000,1770060300,1770060600,1770060900,1770061200,1770061500,1770061800,1770062100],"indicators":{"quote":[{"open":[4.270999908447266,4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.2769999504089355,4.275000095367432,4.2769999504089355,4.279000282287598,4.279000282287598,4.2769999504089355,4.279000282287598,4.275000095367432,4.2729997634887695,4.2729997634887695],"close":[4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.2769999504089355,4.275000095367432,4.2769999504089355,4.279000282287598,4.279000282287598,4.2769999504089355,4.2769999504089355,4.275000095367432,4.2729997634887695,4.2729997634887695,4.275000095367432],"high":[4.2729997634887695,4.2729997634887695,4.275000095367432,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.279000282287598,4.279000282287598,4.279000282287598,4.279000282287598,4.279000282287598,4.275000095367432,4.2729997634887695,4.275000095367432],"volume":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"low":[4.270999908447266,4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.275000095367432,4.275000095367432,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.275000095367432,4.2729997634887695,4.2729997634887695,4.269000053405762]}]}}],"error":null}})";

// Global variables for animation
TimeSeriesGraph* g_graph = nullptr;
unsigned long g_lastUpdate = 0;

// Create vaporwave theme with ALL themeable features enabled
GraphTheme createVaporwaveTheme() {
    GraphTheme theme = {};  // Zero-initialize all fields

    // Basic colors
    theme.backgroundColor = RGB565_DARK_PURPLE;
    theme.lineColor = RGB565_CYAN;
    theme.axisColor = RGB565_MAGENTA;

    // Enable gradient background (3-color at 45 degrees)
    theme.useBackgroundGradient = true;
    theme.backgroundGradient.angle_deg = 45.0f;
    theme.backgroundGradient.color_stops[0] = RGB565_DARK_PURPLE;  // Deep purple
    theme.backgroundGradient.color_stops[1] = RGB565_MAGENTA;       // Magenta
    theme.backgroundGradient.color_stops[2] = 0x4010;               // Dark blue-purple
    theme.backgroundGradient.num_stops = 3;

    // Enable gradient line (horizontal gradient)
    theme.useLineGradient = true;
    theme.lineGradient.angle_deg = 0.0f;  // Horizontal
    theme.lineGradient.color_stops[0] = RGB565_CYAN;
    theme.lineGradient.color_stops[1] = RGB565_MAGENTA;
    theme.lineGradient.num_stops = 2;

    // Set line and axis thickness for smooth rendering
    theme.lineThickness = 2.0f;  // 2% thickness for smooth, visible lines
    theme.axisThickness = 0.8f;  // 0.8% thickness for axes

    // Enable tick marks on Y-axis
    theme.tickColor = RGB565_MAGENTA;
    theme.tickLength = 2.0f;  // 2% tick length

    // Enable pulsing live indicator
    theme.liveIndicatorGradient.center_x = 0.0f;
    theme.liveIndicatorGradient.center_y = 0.0f;
    theme.liveIndicatorGradient.radius = 4.0f;  // 4% radius (larger)
    theme.liveIndicatorGradient.color_stops[0] = RGB565_WHITE;   // Bright center
    theme.liveIndicatorGradient.color_stops[1] = RGB565_CYAN;    // Cyan edge
    theme.liveIndicatorPulseSpeed = 4.0f;  // 4 pulses per second (faster)

    return theme;
}

void displayError(const char* message) {
    // Clear to red background to indicate error
    hal_display_clear(RGB565_RED);
    hal_display_flush();

    Serial.println("=== ERROR ===");
    Serial.println(message);
    Serial.println("=============");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== 10-Year Treasury Bond Tracker ===");
    Serial.println();

    // Initialize display HAL
    Serial.println("[1/5] Initializing display HAL...");
    if (!hal_display_init()) {
        Serial.println("  [FAIL] Display initialization failed");
        displayError("Display initialization failed");
        while (1) delay(1000);
    }
    Serial.println("  [PASS] Display initialized");

    // Apply rotation if configured via build flag
#ifdef APP_DISPLAY_ROTATION
    Serial.printf("  [INFO] Applying rotation: %d degrees\n", APP_DISPLAY_ROTATION);
    hal_display_set_rotation(APP_DISPLAY_ROTATION);
#endif

    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();
    Serial.printf("  [INFO] Display resolution: %d x %d pixels\n", width, height);
    Serial.println();
    delay(500);

    // Initialize relative display abstraction
    Serial.println("[2/5] Initializing relative display abstraction...");
    display_relative_init();
    Serial.println("  [PASS] Relative display initialized");
    Serial.println();
    delay(500);

    // Parse bond data from embedded JSON (Yahoo Chart API format)
    Serial.println("[3/5] Parsing 10-year treasury bond data...");
    Serial.println("  Source: Embedded JSON data (^TNX 5m 1d)");

    YahooChartParser parser("");  // Empty path since we're using parseFromString

    if (!parser.parseFromString(BOND_DATA_JSON)) {
        Serial.println("  [FAIL] Failed to parse bond data");
        displayError("Failed to parse bond data");
        while (1) delay(1000);
    }

    const std::vector<long>& timestamps = parser.getTimestamps();
    const std::vector<double>& closePrices = parser.getClosePrices();

    Serial.printf("  [PASS] Data parsed successfully\n");
    Serial.printf("  [INFO] Data points: %d\n", closePrices.size());
    Serial.printf("  [INFO] First timestamp: %ld\n", timestamps[0]);
    Serial.printf("  [INFO] First yield: %.3f%%\n", closePrices[0]);

    if (closePrices.size() > 1) {
        Serial.printf("  [INFO] Last yield: %.3f%%\n", closePrices[closePrices.size() - 1]);
    }

    Serial.println();
    delay(500);

    // Create TimeSeriesGraph with vaporwave theme
    Serial.println("[4/5] Creating time-series graph...");
    Serial.println("  Theme: Vaporwave (Dark Purple, Cyan, Magenta)");

    GraphTheme theme = createVaporwaveTheme();

    // Debug: Print theme colors
    Serial.printf("  [DEBUG] Background Color: 0x%04X (Dark Purple)\n", theme.backgroundColor);
    Serial.printf("  [DEBUG] Line Color: 0x%04X (should be Cyan 0x07FF)\n", theme.lineColor);
    Serial.printf("  [DEBUG] Axis Color: 0x%04X (should be Magenta 0xF81F)\n", theme.axisColor);
    Serial.printf("  [DEBUG] Line Thickness: %.2f%%\n", theme.lineThickness);
    Serial.printf("  [DEBUG] Axis Thickness: %.2f%%\n", theme.axisThickness);

    // Create graph (use static to keep it alive for loop())
    static TimeSeriesGraph graph(theme);
    g_graph = &graph;  // Store pointer for animation loop

    Serial.println("  [PASS] Graph created");
    Serial.println();
    delay(500);

    // Prepare data for graph
    GraphData graphData;
    graphData.x_values = timestamps;
    graphData.y_values = closePrices;

    graph.setData(graphData);

    // Enable Y-axis tick marks every 0.01 (bond yield increments)
    graph.setYTicks(0.01f);

    // Draw the bond tracker graph
    Serial.println("[5/5] Rendering themeable graph with all features...");
    Serial.println("  Features enabled:");
    Serial.println("    - Gradient background (45-degree, 3-color)");
    Serial.println("    - Gradient data line (horizontal, cyan to magenta)");
    Serial.println("    - Y-axis tick marks (every 0.01)");
    Serial.println("    - Animated pulsing live indicator");

    // Draw background once (static elements)
    Serial.println("  Drawing background...");
    unsigned long bgStart = millis();
    graph.drawBackground();
    unsigned long bgEnd = millis();
    Serial.printf("  [TIME] Background took %lu ms\n", bgEnd - bgStart);

    // Draw initial data
    Serial.println("  Drawing data...");
    unsigned long dataStart = millis();
    graph.drawData();
    unsigned long dataEnd = millis();
    Serial.printf("  [TIME] Data took %lu ms\n", dataEnd - dataStart);

    hal_display_flush();

    Serial.println("  [PASS] Graph rendered");
    Serial.println();

    // Display summary
    Serial.println("=== Themeable Graph Demo Ready ===");
    Serial.println("Visual Verification:");
    Serial.println("  [ ] Gradient background (purple to magenta)");
    Serial.println("  [ ] Magenta axes with tick marks on Y-axis");
    Serial.println("  [ ] Gradient line (cyan to magenta)");
    Serial.println("  [ ] Pulsing white/cyan indicator at last point");
    Serial.println();
    Serial.println("Starting animation loop...");
    Serial.println();

    // Initialize animation timing
    g_lastUpdate = millis();
}

void loop() {
    // Animate the live indicator by calling update() and redrawing data
    unsigned long currentMillis = millis();
    float deltaTime = (currentMillis - g_lastUpdate) / 1000.0f;  // Convert to seconds
    g_lastUpdate = currentMillis;

    if (g_graph != nullptr && deltaTime > 0.001f) {  // Avoid division by zero
        // Update animation state
        g_graph->update(deltaTime);

        // Redraw only the data layer (background is static)
        g_graph->drawData();
        hal_display_flush();
    }

    delay(50);  // ~20 FPS for smooth animation
}
