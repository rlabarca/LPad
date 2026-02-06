/**
 * @file demo_screen.cpp
 * @brief Release 0.5 Demo Application
 *
 * This demo demonstrates the full capabilities of the LPad UI system:
 * - HAL abstraction for hardware independence
 * - Resolution-independent display via RelativeDisplay
 * - Layered rendering with off-screen canvases
 * - Smooth 30fps animation via AnimationTicker
 * - ThemeManager for runtime theming
 * - Gradient backgrounds
 * - Time series graphs
 * - Animated live indicators
 *
 * See features/demo_release_0.5.md for specification.
 */

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "display.h"
#include "relative_display.h"
#include "ui_time_series_graph.h"
#include "yahoo_chart_parser.h"
#include "animation_ticker.h"
#include "theme_manager.h"

// Custom RGB565 colors for the demo
#define RGB565_DARK_PURPLE 0x4810  // Deep purple
#define RGB565_DARK_BLUE   0x001F  // Pure deep blue

// Embedded test data from test_data/yahoo_chart_tnx_5m_1d.json
const char* TEST_DATA_JSON = R"({"chart":{"result":[{"meta":{"currency":"USD","symbol":"^TNX","exchangeName":"CGI","fullExchangeName":"Cboe Indices","instrumentType":"INDEX","firstTradeDate":-252326400,"regularMarketTime":1770062392,"hasPrePostMarketData":false,"gmtoffset":-21600,"timezone":"CST","exchangeTimezoneName":"America/Chicago","regularMarketPrice":4.275,"fiftyTwoWeekHigh":4.997,"fiftyTwoWeekLow":3.345,"regularMarketDayHigh":4.261,"regularMarketDayLow":4.237,"regularMarketVolume":0,"longName":"CBOE Interest Rate 10 Year T No","shortName":"CBOE Interest Rate 10 Year T No","chartPreviousClose":4.227,"previousClose":4.227,"scale":3,"priceHint":4,"currentTradingPeriod":{"pre":{"timezone":"CST","end":1770038400,"start":1770038400,"gmtoffset":-21600},"regular":{"timezone":"CST","end":1770062400,"start":1770038400,"gmtoffset":-21600},"post":{"timezone":"CST","end":1770062400,"start":1770062400,"gmtoffset":-21600}},"tradingPeriods":[[{"timezone":"CST","end":1770062400,"start":1770038400,"gmtoffset":-21600}]],"dataGranularity":"5m","range":"1d","validRanges":["1d","5d","1mo","3mo","6mo","1y","2y","5y","10y","ytd","max"]},"timestamp":[1770057900,1770058200,1770058500,1770058800,1770059100,1770059400,1770059700,1770060000,1770060300,1770060600,1770060900,1770061200,1770061500,1770061800,1770062100],"indicators":{"quote":[{"open":[4.270999908447266,4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.2769999504089355,4.275000095367432,4.2769999504089355,4.279000282287598,4.279000282287598,4.2769999504089355,4.279000282287598,4.275000095367432,4.2729997634887695,4.2729997634887695],"close":[4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.2769999504089355,4.275000095367432,4.2769999504089355,4.279000282287598,4.279000282287598,4.2769999504089355,4.2769999504089355,4.275000095367432,4.2729997634887695,4.2729997634887695,4.275000095367432],"high":[4.2729997634887695,4.2729997634887695,4.275000095367432,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.279000282287598,4.279000282287598,4.279000282287598,4.279000282287598,4.279000282287598,4.275000095367432,4.2729997634887695,4.275000095367432],"volume":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"low":[4.270999908447266,4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.275000095367432,4.275000095367432,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.275000095367432,4.2729997634887695,4.2729997634887695,4.269000053405762]}]}}],"error":null}})";

// Global variables
TimeSeriesGraph* g_graph = nullptr;
AnimationTicker* g_ticker = nullptr;
GraphData g_graphData;  // Store data for graph

// Mode switching state
int g_currentMode = 0;
float g_modeTimer = 0.0f;
const float MODE_SWITCH_INTERVAL = 8.0f;  // Switch modes every 8 seconds

// Create theme with ALL GRADIENTS using ThemeManager colors (Mode 0)
GraphTheme createGradientTheme() {
    GraphTheme theme = {};

    // Get colors from ThemeManager (default theme)
    const LPad::Theme* lpadTheme = LPad::ThemeManager::getInstance().getTheme();

    // Basic colors from ThemeManager
    theme.backgroundColor = lpadTheme->colors.background;
    theme.lineColor = lpadTheme->colors.primary;
    theme.axisColor = lpadTheme->colors.secondary;

    // Enable 45-degree background gradient using theme colors
    // Background (darker) -> Secondary (lighter)
    theme.useBackgroundGradient = true;
    theme.backgroundGradient.angle_deg = 45.0f;
    theme.backgroundGradient.color_stops[0] = lpadTheme->colors.background;
    theme.backgroundGradient.color_stops[1] = lpadTheme->colors.secondary;
    theme.backgroundGradient.num_stops = 2;

    // Enable gradient line (horizontal, Primary -> Accent)
    // Primary (left/old data) -> Accent (right/new data)
    theme.useLineGradient = true;
    theme.lineGradient.angle_deg = 0.0f;
    theme.lineGradient.color_stops[0] = lpadTheme->colors.primary;
    theme.lineGradient.color_stops[1] = lpadTheme->colors.accent;
    theme.lineGradient.num_stops = 2;

    // Line and axis styling
    theme.lineThickness = 2.0f;
    theme.axisThickness = 0.8f;
    theme.tickColor = lpadTheme->colors.graph_ticks;
    theme.tickLength = 2.5f;

    // Enable integrated live indicator with radial gradient
    // Accent (center) -> Primary (outer edge) as per spec
    theme.liveIndicatorGradient.color_stops[0] = lpadTheme->colors.accent;
    theme.liveIndicatorGradient.color_stops[1] = lpadTheme->colors.primary;
    theme.liveIndicatorPulseSpeed = 0.5f;  // 0.5 pulses per second (2 second cycle)

    return theme;
}

// Create theme with ALL SOLID COLORS (Mode 1)
GraphTheme createSolidTheme() {
    GraphTheme theme = {};

    // Solid dark grey background
    theme.backgroundColor = 0x2104;  // Dark grey RGB565
    theme.useBackgroundGradient = false;

    // Solid white line
    theme.lineColor = RGB565_WHITE;
    theme.useLineGradient = false;

    // Magenta axes (keep for visibility)
    theme.axisColor = RGB565_MAGENTA;

    // Line and axis styling
    theme.lineThickness = 2.0f;
    theme.axisThickness = 0.8f;
    theme.tickColor = RGB565_CYAN;
    theme.tickLength = 2.5f;

    // Solid green indicator
    theme.liveIndicatorGradient.color_stops[0] = RGB565_GREEN;
    theme.liveIndicatorGradient.color_stops[1] = RGB565_GREEN;  // Same color = solid
    theme.liveIndicatorPulseSpeed = 0.5f;

    return theme;
}

// Create theme with MIXED MODE (Mode 2)
GraphTheme createMixedTheme() {
    GraphTheme theme = {};

    // Solid background
    theme.backgroundColor = RGB565_DARK_BLUE;
    theme.useBackgroundGradient = false;

    // Gradient line
    theme.useLineGradient = true;
    theme.lineGradient.angle_deg = 0.0f;
    theme.lineGradient.color_stops[0] = RGB565_YELLOW;
    theme.lineGradient.color_stops[1] = RGB565_RED;
    theme.lineGradient.num_stops = 2;

    // Cyan axes
    theme.axisColor = RGB565_CYAN;

    // Line and axis styling
    theme.lineThickness = 2.0f;
    theme.axisThickness = 0.8f;
    theme.tickColor = RGB565_WHITE;
    theme.tickLength = 2.5f;

    // Gradient indicator
    theme.liveIndicatorGradient.color_stops[0] = RGB565_YELLOW;
    theme.liveIndicatorGradient.color_stops[1] = RGB565_RED;
    theme.liveIndicatorPulseSpeed = 0.5f;

    return theme;
}

void displayError(const char* message) {
    hal_display_clear(RGB565_RED);
    hal_display_flush();
    Serial.println("=== ERROR ===");
    Serial.println(message);
    Serial.println("=============");
}

// Draw the "V0.5 DEMO" title using ThemeManager
void drawTitle() {
    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();
    Arduino_GFX* gfx = static_cast<Arduino_GFX*>(hal_display_get_gfx());

    if (gfx == nullptr) return;

    // Get display dimensions
    int32_t width = hal_display_get_width_pixels();

    // Set font and color from theme
    gfx->setFont(theme->fonts.heading);
    gfx->setTextColor(theme->colors.text_main);

    // Calculate text position (centered at top)
    const char* title = "V0.5 DEMO";
    int16_t x1, y1;
    uint16_t w, h;
    gfx->getTextBounds(title, 0, 0, &x1, &y1, &w, &h);

    int16_t text_x = (width - w) / 2;
    int16_t text_y = 10 + h;  // 10px from top + text height

    // Draw title
    gfx->setCursor(text_x, text_y);
    gfx->print(title);
}

void setup() {
    Serial.begin(115200);
    delay(2000);  // Longer delay for ESP32-S3 USB CDC

    Serial.println("\n\n\n=== LPad Base UI Demo Application ===");
    Serial.println("DEBUG: Entered setup()");
    Serial.flush();
    delay(100);

    // [1/6] Initialize display HAL
    Serial.println("[1/6] Initializing display HAL...");
    Serial.flush();
    delay(100);

    Serial.println("DEBUG: About to call hal_display_init()");
    Serial.flush();

    bool init_result = hal_display_init();
    Serial.printf("DEBUG: hal_display_init() returned: %d\n", init_result);
    Serial.flush();

    if (!init_result) {
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
    Serial.flush();

    Serial.println("DEBUG: About to call display_relative_init()");
    Serial.flush();
    display_relative_init();

    Serial.println("DEBUG: About to get GFX object");
    Serial.flush();
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
    Serial.flush();

    Serial.println("DEBUG: About to create AnimationTicker");
    Serial.flush();
    static AnimationTicker ticker(30);
    g_ticker = &ticker;
    Serial.println("DEBUG: AnimationTicker created");
    Serial.flush();
    Serial.println("  [PASS] AnimationTicker created (30fps)");
    Serial.println();
    delay(500);

    // [4/6] Parse test data
    Serial.println("[4/6] Parsing test data from embedded JSON...");
    Serial.flush();

    Serial.println("DEBUG: About to create YahooChartParser");
    Serial.flush();
    YahooChartParser parser("");

    Serial.println("DEBUG: About to parse JSON");
    Serial.flush();
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
    Serial.flush();

    // TimeSeriesGraph with layered rendering and integrated live indicator
    Serial.println("  Creating TimeSeriesGraph with mode-switching demo...");
    Serial.flush();

    Serial.println("DEBUG: About to create initial gradient theme");
    Serial.flush();
    GraphTheme theme = createGradientTheme();

    Serial.println("DEBUG: About to create TimeSeriesGraph object");
    Serial.printf("DEBUG: Display dimensions: %d x %d\n", width, height);
    Serial.flush();
    static TimeSeriesGraph graph(theme, display, width, height);
    g_graph = &graph;

    Serial.println("DEBUG: TimeSeriesGraph object created, about to call begin()");
    Serial.flush();

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

    // Draw title using ThemeManager (spec requirement)
    Serial.println("  Drawing title...");
    drawTitle();

    hal_display_flush();

    Serial.println("  [PASS] Initial render complete");
    Serial.println();

    Serial.println("=== Release 0.5 Demo Application Ready ===");
    Serial.println();
    Serial.println("Visual Elements (using ThemeManager colors):");
    Serial.println("  [x] Title: 'V0.5 DEMO' (ThemeFonts.heading, ThemeColors.text_main)");
    Serial.println("  [x] Background: 45-degree gradient (background->secondary)");
    Serial.println("  [x] Graph Line: Gradient (primary->accent)");
    Serial.println("  [x] Live Indicator: Pulsing at 30fps (accent->primary)");
    Serial.println("  [x] Y-Axis Labels: ThemeFonts.smallest");
    Serial.println();
    Serial.println("Mode Switching Demo - Tests gradient and solid rendering:");
    Serial.println();
    Serial.println("Mode 0 (ThemeManager colors with gradients)");
    Serial.println("Mode 1 (Solid colors)");
    Serial.println("Mode 2 (Mixed mode)");
    Serial.println();
    Serial.printf("Switching modes every %.0f seconds...\n", MODE_SWITCH_INTERVAL);
    Serial.println("Starting 30fps animation loop...");
    Serial.println();
}

void loop() {
    // Wait for next frame and get deltaTime
    float deltaTime = g_ticker->waitForNextFrame();

    if (g_graph == nullptr) {
        return;
    }

    // Mode switching logic - cycle through different visual styles
    g_modeTimer += deltaTime;
    if (g_modeTimer >= MODE_SWITCH_INTERVAL) {
        g_modeTimer = 0.0f;
        g_currentMode = (g_currentMode + 1) % 3;  // Cycle through 3 modes

        // Create new theme based on mode
        GraphTheme newTheme;
        const char* modeName;
        switch (g_currentMode) {
            case 0:
                newTheme = createGradientTheme();
                modeName = "ALL GRADIENTS";
                break;
            case 1:
                newTheme = createSolidTheme();
                modeName = "ALL SOLID";
                break;
            case 2:
                newTheme = createMixedTheme();
                modeName = "MIXED";
                break;
            default:
                newTheme = createGradientTheme();
                modeName = "DEFAULT";
                break;
        }

        Serial.printf("\n>>> Switching to Mode %d: %s <<<\n\n", g_currentMode, modeName);

        // Update the graph's theme
        g_graph->setTheme(newTheme);

        // Redraw static layers with new theme
        g_graph->drawBackground();
        g_graph->drawData();
        g_graph->render();

        // Redraw title on top
        drawTitle();

        hal_display_flush();
    }

    // Update the graph with integrated live indicator animation
    // This uses dirty-rect optimization to avoid flashing
    g_graph->update(deltaTime);

    // Flush to ensure display updates
    hal_display_flush();
}
