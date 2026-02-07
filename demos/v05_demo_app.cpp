/**
 * @file v05_demo_app.cpp
 * @brief Release 0.5 Demo Application Implementation
 */

#include "v05_demo_app.h"
#include "theme_manager.h"
#include "display.h"
#include <Arduino_GFX_Library.h>

// Embedded test data from test_data/yahoo_chart_tnx_5m_1d.json
const char* V05_TEST_DATA_JSON = R"({"chart":{"result":[{"meta":{"currency":"USD","symbol":"^TNX","exchangeName":"CGI","fullExchangeName":"Cboe Indices","instrumentType":"INDEX","firstTradeDate":-252326400,"regularMarketTime":1770062392,"hasPrePostMarketData":false,"gmtoffset":-21600,"timezone":"CST","exchangeTimezoneName":"America/Chicago","regularMarketPrice":4.275,"fiftyTwoWeekHigh":4.997,"fiftyTwoWeekLow":3.345,"regularMarketDayHigh":4.261,"regularMarketDayLow":4.237,"regularMarketVolume":0,"longName":"CBOE Interest Rate 10 Year T No","shortName":"CBOE Interest Rate 10 Year T No","chartPreviousClose":4.227,"previousClose":4.227,"scale":3,"priceHint":4,"currentTradingPeriod":{"pre":{"timezone":"CST","end":1770038400,"start":1770038400,"gmtoffset":-21600},"regular":{"timezone":"CST","end":1770062400,"start":1770038400,"gmtoffset":-21600},"post":{"timezone":"CST","end":1770062400,"start":1770062400,"gmtoffset":-21600}},"tradingPeriods":[[{"timezone":"CST","end":1770062400,"start":1770038400,"gmtoffset":-21600}]],"dataGranularity":"5m","range":"1d","validRanges":["1d","5d","1mo","3mo","6mo","1y","2y","5y","10y","ytd","max"]},"timestamp":[1770057900,1770058200,1770058500,1770058800,1770059100,1770059400,1770059700,1770060000,1770060300,1770060600,1770060900,1770061200,1770061500,1770061800,1770062100],"indicators":{"quote":[{"open":[4.270999908447266,4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.2769999504089355,4.275000095367432,4.2769999504089355,4.279000282287598,4.279000282287598,4.2769999504089355,4.279000282287598,4.275000095367432,4.2729997634887695,4.2729997634887695],"close":[4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.2769999504089355,4.275000095367432,4.2769999504089355,4.279000282287598,4.279000282287598,4.2769999504089355,4.2769999504089355,4.275000095367432,4.2729997634887695,4.2729997634887695,4.275000095367432],"high":[4.2729997634887695,4.2729997634887695,4.275000095367432,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.279000282287598,4.279000282287598,4.279000282287598,4.279000282287598,4.279000282287598,4.275000095367432,4.2729997634887695,4.275000095367432],"volume":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"low":[4.270999908447266,4.270999908447266,4.2729997634887695,4.275000095367432,4.275000095367432,4.275000095367432,4.275000095367432,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.2769999504089355,4.275000095367432,4.2729997634887695,4.2729997634887695,4.269000053405762]}]}}],"error":null}})";

V05DemoApp::V05DemoApp()
    : m_currentStage(STAGE_LOGO)
    , m_display(nullptr)
    , m_graph(nullptr)
    , m_logoScreen(nullptr)
    , m_titleText("DEMO v0.5")
    , m_titleBuffer(nullptr)
    , m_titleBufferX(0)
    , m_titleBufferY(0)
    , m_titleBufferWidth(0)
    , m_titleBufferHeight(0)
    , m_titleBufferValid(false)
    , m_currentMode(0)
    , m_modesShown(0)
    , m_logoHoldTimer(0.0f)
    , m_modeTimer(0.0f)
{
}

V05DemoApp::~V05DemoApp() {
    delete m_graph;
    delete m_logoScreen;
    if (m_titleBuffer != nullptr) {
        free(m_titleBuffer);
        m_titleBuffer = nullptr;
    }
}

bool V05DemoApp::begin(RelativeDisplay* display) {
    if (display == nullptr) {
        Serial.println("[V05DemoApp] ERROR: display is nullptr");
        return false;
    }

    Serial.println("[V05DemoApp] begin() called");
    m_display = display;

    // Get theme colors
    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();

    // Parse test data
    YahooChartParser parser("");
    if (!parser.parseFromString(V05_TEST_DATA_JSON)) {
        Serial.println("[V05DemoApp] ERROR: Failed to parse test data");
        return false;
    }

    m_graphData.x_values = parser.getTimestamps();
    m_graphData.y_values = parser.getClosePrices();
    Serial.printf("[V05DemoApp] Parsed %d data points\n", m_graphData.y_values.size());

    // Create LogoScreen with 2s wait, 1.5s animation
    Serial.println("[V05DemoApp] Creating LogoScreen...");
    m_logoScreen = new LogoScreen(2.0f, 1.5f);
    Serial.printf("[V05DemoApp] Calling logoScreen->begin() with background color 0x%04X\n", theme->colors.background);
    if (!m_logoScreen->begin(m_display, theme->colors.background)) {
        Serial.println("[V05DemoApp] ERROR: LogoScreen initialization failed");
        return false;
    }
    Serial.println("[V05DemoApp] LogoScreen initialized successfully");

    // Create TimeSeriesGraph
    Arduino_GFX* gfx = static_cast<Arduino_GFX*>(hal_display_get_gfx());
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    GraphTheme graphTheme = createGradientTheme();
    m_graph = new TimeSeriesGraph(graphTheme, gfx, width, height);

    if (!m_graph->begin()) {
        Serial.println("[V05DemoApp] ERROR: Graph initialization failed");
        return false;
    }

    m_graph->setData(m_graphData);
    m_graph->setYTicks(0.002f);

    Serial.println("[V05DemoApp] Initialized successfully");
    return true;
}

void V05DemoApp::update(float deltaTime) {
    switch (m_currentStage) {
        case STAGE_LOGO: {
            // Update logo animation
            LogoScreen::State logoState = m_logoScreen->update(deltaTime);

            // Check if animation is done and hold timer expired
            if (logoState == LogoScreen::State::DONE) {
                m_logoHoldTimer += deltaTime;
                if (m_logoHoldTimer >= LOGO_HOLD_DURATION) {
                    transitionToStage(STAGE_GRAPH_CYCLE);
                }
            }
            break;
        }

        case STAGE_GRAPH_CYCLE: {
            // Update graph animation (live indicator)
            if (m_graph != nullptr) {
                m_graph->update(deltaTime);
            }

            // Check if it's time to switch to next mode
            m_modeTimer += deltaTime;
            if (m_modeTimer >= MODE_DURATION) {
                switchToNextMode();
            }
            break;
        }

        case STAGE_FINISHED:
            // Do nothing, wait for external reset or query via isFinished()
            break;
    }
}

void V05DemoApp::render() {
    switch (m_currentStage) {
        case STAGE_LOGO:
            // LogoScreen handles its own rendering via update()
            break;

        case STAGE_GRAPH_CYCLE:
            // Graph stage: only flush display (update() already drew live indicator)
            // Full render + title are done once during mode transitions
            hal_display_flush();
            break;

        case STAGE_FINISHED:
            // Keep last frame visible
            break;
    }
}

bool V05DemoApp::isFinished() const {
    return m_currentStage == STAGE_FINISHED;
}

void V05DemoApp::setTitle(const char* title) {
    m_titleText = title;
    m_titleBufferValid = false;  // Invalidate buffer when title changes
}

void V05DemoApp::drawTitle() {
    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();
    Arduino_GFX* gfx = static_cast<Arduino_GFX*>(hal_display_get_gfx());

    if (gfx == nullptr) return;

    // Get display dimensions
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    // Set font and color from theme (using normal 12pt font, not heading 24pt)
    gfx->setFont(theme->fonts.normal);
    gfx->setTextColor(theme->colors.text_main);

    // Calculate text position (top-left with 5% padding)
    int16_t x1, y1;
    uint16_t w, h;
    gfx->getTextBounds(m_titleText, 0, 0, &x1, &y1, &w, &h);

    int16_t padding_x = width * 0.05f;
    int16_t padding_y = height * 0.05f;
    int16_t text_x = padding_x;
    int16_t text_y = padding_y + h;  // 5% from top + text height

    // Draw title (left-justified)
    gfx->setCursor(text_x, text_y);
    gfx->print(m_titleText);
}

void V05DemoApp::transitionToStage(Stage newStage) {
    m_currentStage = newStage;
    m_modeTimer = 0.0f;
    m_logoHoldTimer = 0.0f;

    switch (newStage) {
        case STAGE_GRAPH_CYCLE:
            Serial.println("[V05DemoApp] Transitioning to STAGE_GRAPH_CYCLE");
            m_currentMode = 0;
            m_modesShown = 0;

            // Configure and render first graph mode (Mode 0: Scientific + Gradient)
            if (m_graph != nullptr) {
                GraphTheme theme = createGradientTheme();
                m_graph->setTheme(theme);
                m_graph->setTickLabelPosition(TickLabelPosition::OUTSIDE);
                m_graph->setXAxisTitle("TIME (5m)");
                m_graph->setYAxisTitle("YIELD (%)");
                m_graph->drawBackground();
                m_graph->drawData();
                m_graph->render();
                drawTitle();
                hal_display_flush();
                Serial.println("[V05DemoApp] >>> Mode 0: SCIENTIFIC + GRADIENT <<<");
            }
            break;

        case STAGE_FINISHED:
            Serial.println("[V05DemoApp] Demo cycle FINISHED (all 6 modes shown)");
            break;

        default:
            break;
    }
}

GraphTheme V05DemoApp::createGradientTheme() {
    GraphTheme theme = {};

    // Get colors from ThemeManager (default theme)
    const LPad::Theme* lpadTheme = LPad::ThemeManager::getInstance().getTheme();

    // Basic colors from ThemeManager
    theme.backgroundColor = lpadTheme->colors.background;
    theme.lineColor = lpadTheme->colors.primary;
    theme.axisColor = lpadTheme->colors.secondary;

    // Enable 45-degree background gradient using theme colors
    theme.useBackgroundGradient = true;
    theme.backgroundGradient.angle_deg = 45.0f;
    theme.backgroundGradient.color_stops[0] = lpadTheme->colors.background;
    theme.backgroundGradient.color_stops[1] = lpadTheme->colors.secondary;
    theme.backgroundGradient.num_stops = 2;

    // Enable gradient line (horizontal, Primary -> Accent)
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
    theme.liveIndicatorGradient.color_stops[0] = lpadTheme->colors.accent;
    theme.liveIndicatorGradient.color_stops[1] = lpadTheme->colors.primary;
    theme.liveIndicatorPulseSpeed = 0.5f;

    // Font assignments from ThemeManager
    theme.tickFont = lpadTheme->fonts.smallest;
    theme.axisTitleFont = lpadTheme->fonts.ui;

    return theme;
}

GraphTheme V05DemoApp::createSolidTheme() {
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

GraphTheme V05DemoApp::createMixedTheme() {
    GraphTheme theme = {};

    // Solid background
    theme.backgroundColor = 0x001F;  // Dark blue RGB565
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

void V05DemoApp::switchToNextMode() {
    m_modeTimer = 0.0f;
    m_modesShown++;

    // Check if we've shown all 6 modes
    if (m_modesShown >= 6) {
        transitionToStage(STAGE_FINISHED);
        return;
    }

    // Advance to next mode
    m_currentMode = (m_currentMode + 1) % 6;

    // Visual mode: 0=Gradient, 1=Solid, 2=Mixed
    int visualMode = m_currentMode % 3;
    // Layout mode: 0=Scientific (modes 0-2), 1=Compact (modes 3-5)
    int layoutMode = m_currentMode / 3;

    if (m_graph == nullptr) return;

    // Apply visual theme
    GraphTheme newTheme;
    const char* visualName;
    switch (visualMode) {
        case 0:  newTheme = createGradientTheme(); visualName = "GRADIENT"; break;
        case 1:  newTheme = createSolidTheme();    visualName = "SOLID";    break;
        default: newTheme = createMixedTheme();    visualName = "MIXED";    break;
    }
    m_graph->setTheme(newTheme);

    // Apply layout mode
    const char* layoutName;
    if (layoutMode == 0) {
        layoutName = "SCIENTIFIC";
        m_graph->setTickLabelPosition(TickLabelPosition::OUTSIDE);
        m_graph->setXAxisTitle("TIME (5m)");
        m_graph->setYAxisTitle("YIELD (%)");
    } else {
        layoutName = "COMPACT";
        m_graph->setTickLabelPosition(TickLabelPosition::INSIDE);
        m_graph->setXAxisTitle(nullptr);
        m_graph->setYAxisTitle(nullptr);
    }

    Serial.printf("[V05DemoApp] >>> Mode %d: %s + %s <<<\n", m_currentMode, layoutName, visualName);

    // Redraw static layers with new theme and layout
    m_graph->drawBackground();
    m_graph->drawData();
    m_graph->render();
    drawTitle();
    hal_display_flush();
}

void V05DemoApp::renderTitleToBuffer() {
    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();
    Arduino_GFX* gfx = static_cast<Arduino_GFX*>(hal_display_get_gfx());

    if (gfx == nullptr) return;

    // Get display dimensions
    int32_t width = hal_display_get_width_pixels();
    int32_t height = hal_display_get_height_pixels();

    // Set font and get text bounds
    gfx->setFont(theme->fonts.normal);
    int16_t x1, y1;
    uint16_t w, h;
    gfx->getTextBounds(m_titleText, 0, 0, &x1, &y1, &w, &h);

    // Calculate text position (5% padding from top-left)
    int16_t padding_x = width * 0.05f;
    int16_t padding_y = height * 0.05f;

    // Allocate buffer with some extra margin for safety (add 4 pixels margin on all sides)
    int16_t margin = 4;
    m_titleBufferWidth = w + (margin * 2);
    m_titleBufferHeight = h + (margin * 2);
    m_titleBufferX = padding_x - margin;
    m_titleBufferY = padding_y - margin;

    // Reallocate buffer if size changed
    if (m_titleBuffer != nullptr) {
        free(m_titleBuffer);
    }
    m_titleBuffer = static_cast<uint16_t*>(malloc(m_titleBufferWidth * m_titleBufferHeight * sizeof(uint16_t)));

    if (m_titleBuffer == nullptr) {
        Serial.println("[V05DemoApp] ERROR: Failed to allocate title buffer");
        return;
    }

    // Create a temporary canvas to render the title
    Arduino_Canvas* titleCanvas = new Arduino_Canvas(m_titleBufferWidth, m_titleBufferHeight, gfx);
    if (titleCanvas == nullptr) {
        Serial.println("[V05DemoApp] ERROR: Failed to create title canvas");
        free(m_titleBuffer);
        m_titleBuffer = nullptr;
        return;
    }

    titleCanvas->begin();

    // Fill with background color (transparent-ish, will be overwritten by text)
    titleCanvas->fillScreen(theme->colors.background);

    // Draw title text
    titleCanvas->setFont(theme->fonts.normal);
    titleCanvas->setTextColor(theme->colors.text_main);
    titleCanvas->setCursor(margin, margin + h);  // Position within canvas (accounting for margin)
    titleCanvas->print(m_titleText);

    // Copy rendered content to buffer
    uint16_t* canvasBuffer = titleCanvas->getFramebuffer();
    if (canvasBuffer != nullptr) {
        memcpy(m_titleBuffer, canvasBuffer, m_titleBufferWidth * m_titleBufferHeight * sizeof(uint16_t));
        m_titleBufferValid = true;
    } else {
        Serial.println("[V05DemoApp] ERROR: Failed to get canvas framebuffer");
        m_titleBufferValid = false;
    }

    delete titleCanvas;
}

void V05DemoApp::blitTitle() {
    // Render title to buffer if not already done
    if (!m_titleBufferValid) {
        renderTitleToBuffer();
    }

    // Blit buffer to display using fast DMA
    if (m_titleBuffer != nullptr && m_titleBufferValid) {
        hal_display_fast_blit(m_titleBufferX, m_titleBufferY,
                              m_titleBufferWidth, m_titleBufferHeight,
                              m_titleBuffer);
    }
}
