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
 * - Vector asset rendering (logo splash screen)
 *
 * See features/demo_release_0.5.md for specification.
 */

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "display.h"
#include "relative_display.h"
#include "ui_time_series_graph.h"
#include "ui_logo_screen.h"
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
RelativeDisplay* g_relativeDisplay = nullptr;
LogoScreen* g_logoScreen = nullptr;

// Demo state machine - shows logo, then cycles through all graph modes
enum DemoPhase {
    PHASE_LOGO_ANIMATION,  // Animated LPad logo (wait + animate + hold)
    PHASE_GRAPH_CYCLE      // Cycle through all 6 graph modes
};
DemoPhase g_currentPhase = PHASE_LOGO_ANIMATION;
float g_logoHoldTimer = 0.0f;
const float LOGO_HOLD_DURATION = 2.0f;  // Hold final logo position for 2 seconds
const float MODE_SWITCH_INTERVAL = 5.0f;  // Show each graph mode for 5 seconds
float g_modeTimer = 0.0f;

// Mode cycling (6 combinations: 2 layouts x 3 visual styles)
int g_currentMode = 0;       // 0-5: cycles through 6 combinations
int g_modesShown = 0;        // Track how many modes shown in current cycle

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

    // Font assignments from ThemeManager
    theme.tickFont = lpadTheme->fonts.smallest;       // 9pt for tick labels
    theme.axisTitleFont = lpadTheme->fonts.ui;         // 18pt for axis titles

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

    // Font assignments from ThemeManager
    const LPad::Theme* lpadTheme = LPad::ThemeManager::getInstance().getTheme();
    theme.tickFont = lpadTheme->fonts.smallest;
    theme.axisTitleFont = lpadTheme->fonts.ui;

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

    // Font assignments from ThemeManager
    const LPad::Theme* lpadTheme = LPad::ThemeManager::getInstance().getTheme();
    theme.tickFont = lpadTheme->fonts.smallest;
    theme.axisTitleFont = lpadTheme->fonts.ui;

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
    delay(500);  // Brief delay for ESP32-S3 USB CDC
    yield();

    Serial.println("\n\n\n=== LPad Base UI Demo Application ===");
    Serial.println("DEBUG: Entered setup()");
    Serial.flush();
    yield();

    // [1/6] Initialize display HAL
    Serial.println("[1/6] Initializing display HAL...");
    Serial.flush();
    yield();

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
    yield();

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

    // Create RelativeDisplay wrapper for vector rendering
    static RelativeDisplay relDisplay(display, width, height);
    g_relativeDisplay = &relDisplay;
    g_relativeDisplay->init();

    Serial.println("  [PASS] RelativeDisplay initialized");
    Serial.println();
    yield();

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
    yield();

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
    yield();

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
    graph.setTickLabelPosition(TickLabelPosition::OUTSIDE);
    graph.setXAxisTitle("TIME (5m)");
    graph.setYAxisTitle("YIELD (%)");
    Serial.println("  [PASS] TimeSeriesGraph created with integrated indicator");
    Serial.println();
    yield();

    // [6/6] Create LogoScreen and render initial frame
    Serial.println("[6/6] Creating LogoScreen animation...");

    // Get theme colors
    const LPad::Theme* theme_ptr = LPad::ThemeManager::getInstance().getTheme();

    // Create LogoScreen with 2s wait, 1.5s animation
    static LogoScreen logoScreen(2.0f, 1.5f);
    g_logoScreen = &logoScreen;

    // Initialize with display and background color
    if (!g_logoScreen->begin(g_relativeDisplay, theme_ptr->colors.background)) {
        displayError("LogoScreen initialization failed");
        while (1) delay(1000);
    }

    Serial.println("  [PASS] LogoScreen initialized with dirty-rect optimization");
    Serial.println();

    Serial.println("=== Release 0.5 Demo Application Ready ===");
    Serial.println();
    Serial.println("Demo Cycle (repeats indefinitely):");
    Serial.println("  Logo Animation → All 6 Graph Modes → Logo Again → Repeat");
    Serial.println();
    Serial.println("Sequence:");
    Serial.println("  1. Logo: Wait 2s + Animate 1.5s + Hold 2s (5.5s total)");
    Serial.println("  2. Graph Modes: Cycle through all 6 modes @ 5s each (30s total)");
    Serial.println("  3. Return to step 1");
    Serial.println();
    Serial.println("Graph Modes (2 layouts × 3 visual styles = 6 combinations):");
    Serial.println();
    Serial.println("  SCIENTIFIC Layout (OUTSIDE labels + axis titles):");
    Serial.println("    Mode 0: Gradient theme");
    Serial.println("    Mode 1: Solid colors");
    Serial.println("    Mode 2: Mixed (solid bg + gradient line)");
    Serial.println();
    Serial.println("  COMPACT Layout (INSIDE labels, no titles):");
    Serial.println("    Mode 3: Gradient theme");
    Serial.println("    Mode 4: Solid colors");
    Serial.println("    Mode 5: Mixed");
    Serial.println();
    Serial.println("Visual Elements:");
    Serial.println("  [x] Animated Vector Logo (start: 75% height, end: 5% height)");
    Serial.println("  [x] Dirty-rect optimization (flicker-free)");
    Serial.println("  [x] Smooth 30fps animation");
    Serial.println("  [x] Aspect ratio preservation");
    Serial.println();
    Serial.println("Total cycle time: ~35.5 seconds");
    Serial.println();
    Serial.println("Starting animation loop...");
    Serial.println();
}

void loop() {
    // Wait for next frame and get deltaTime
    float deltaTime = g_ticker->waitForNextFrame();

    // Phase state machine - logo animation, then cycle through all 6 graph modes
    switch (g_currentPhase) {
        case PHASE_LOGO_ANIMATION: {
            // Update logo animation (handles rendering internally with dirty-rect)
            LogoScreen::State logoState = g_logoScreen->update(deltaTime);

            // Check if animation is done and hold timer expired
            if (logoState == LogoScreen::State::DONE) {
                g_logoHoldTimer += deltaTime;
                if (g_logoHoldTimer >= LOGO_HOLD_DURATION) {
                    // Transition to graph cycle phase
                    Serial.println("\n=== Starting Graph Mode Cycle (6 modes) ===\n");
                    g_currentPhase = PHASE_GRAPH_CYCLE;
                    g_modeTimer = 0.0f;
                    g_logoHoldTimer = 0.0f;
                    g_currentMode = 0;  // Start from mode 0
                    g_modesShown = 0;

                    // Configure and render first graph mode
                    GraphTheme newTheme = createGradientTheme();
                    g_graph->setTheme(newTheme);
                    g_graph->setTickLabelPosition(TickLabelPosition::OUTSIDE);
                    g_graph->setXAxisTitle("TIME (5m)");
                    g_graph->setYAxisTitle("YIELD (%)");

                    Serial.println(">>> Mode 0: SCIENTIFIC + GRADIENT <<<");

                    g_graph->drawBackground();
                    g_graph->drawData();
                    g_graph->render();
                    drawTitle();
                    hal_display_flush();
                }
            }
            break;
        }

        case PHASE_GRAPH_CYCLE: {
            if (g_graph == nullptr) return;

            // Update graph animation (live indicator)
            g_graph->update(deltaTime);
            hal_display_flush();

            // Check if it's time to switch to next mode
            g_modeTimer += deltaTime;
            if (g_modeTimer >= MODE_SWITCH_INTERVAL) {
                g_modeTimer = 0.0f;
                g_modesShown++;

                // Check if we've shown all 6 modes
                if (g_modesShown >= 6) {
                    // Return to logo animation
                    Serial.println("\n=== All 6 modes shown, returning to Logo Animation ===\n");
                    g_logoScreen->reset();
                    g_currentPhase = PHASE_LOGO_ANIMATION;
                    g_modesShown = 0;
                } else {
                    // Advance to next mode
                    g_currentMode = (g_currentMode + 1) % 6;

                    // Visual mode: 0=Gradient, 1=Solid, 2=Mixed
                    int visualMode = g_currentMode % 3;
                    // Layout mode: 0=Scientific (modes 0-2), 1=Compact (modes 3-5)
                    int layoutMode = g_currentMode / 3;

                    // Apply visual theme
                    GraphTheme newTheme;
                    const char* visualName;
                    switch (visualMode) {
                        case 0:  newTheme = createGradientTheme(); visualName = "GRADIENT"; break;
                        case 1:  newTheme = createSolidTheme();    visualName = "SOLID";    break;
                        default: newTheme = createMixedTheme();    visualName = "MIXED";    break;
                    }
                    g_graph->setTheme(newTheme);

                    // Apply layout mode
                    const char* layoutName;
                    if (layoutMode == 0) {
                        layoutName = "SCIENTIFIC";
                        g_graph->setTickLabelPosition(TickLabelPosition::OUTSIDE);
                        g_graph->setXAxisTitle("TIME (5m)");
                        g_graph->setYAxisTitle("YIELD (%)");
                    } else {
                        layoutName = "COMPACT";
                        g_graph->setTickLabelPosition(TickLabelPosition::INSIDE);
                        g_graph->setXAxisTitle(nullptr);
                        g_graph->setYAxisTitle(nullptr);
                    }

                    Serial.printf("\n>>> Mode %d: %s + %s <<<\n\n", g_currentMode, layoutName, visualName);

                    // Redraw static layers with new theme and layout
                    g_graph->drawBackground();
                    g_graph->drawData();
                    g_graph->render();
                    drawTitle();
                    hal_display_flush();
                }
            }
            break;
        }
    }
}
