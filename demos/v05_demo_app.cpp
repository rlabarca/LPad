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
    , m_logoHoldTimer(0.0f)
    , m_stageTimer(0.0f)
{
}

V05DemoApp::~V05DemoApp() {
    delete m_graph;
    delete m_logoScreen;
}

bool V05DemoApp::begin(RelativeDisplay* display) {
    if (display == nullptr) {
        Serial.println("[V05DemoApp] ERROR: display is nullptr");
        return false;
    }

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
    m_logoScreen = new LogoScreen(2.0f, 1.5f);
    if (!m_logoScreen->begin(m_display, theme->colors.background)) {
        Serial.println("[V05DemoApp] ERROR: LogoScreen initialization failed");
        return false;
    }

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
                    transitionToStage(STAGE_SCIENTIFIC);
                }
            }
            break;
        }

        case STAGE_SCIENTIFIC:
        case STAGE_COMPACT: {
            // Update graph animation (live indicator)
            if (m_graph != nullptr) {
                m_graph->update(deltaTime);
            }

            // Check if it's time to switch stages
            m_stageTimer += deltaTime;
            if (m_stageTimer >= STAGE_DURATION) {
                if (m_currentStage == STAGE_SCIENTIFIC) {
                    transitionToStage(STAGE_COMPACT);
                } else {
                    transitionToStage(STAGE_FINISHED);
                }
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

        case STAGE_SCIENTIFIC:
        case STAGE_COMPACT:
            // Render graph and title
            if (m_graph != nullptr) {
                m_graph->render();
                drawTitle();
                hal_display_flush();
            }
            break;

        case STAGE_FINISHED:
            // Keep last frame visible
            break;
    }
}

bool V05DemoApp::isFinished() const {
    return m_currentStage == STAGE_FINISHED;
}

void V05DemoApp::drawTitle() {
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

void V05DemoApp::transitionToStage(Stage newStage) {
    m_currentStage = newStage;
    m_stageTimer = 0.0f;
    m_logoHoldTimer = 0.0f;

    switch (newStage) {
        case STAGE_SCIENTIFIC:
            Serial.println("[V05DemoApp] Transitioning to STAGE_SCIENTIFIC");
            if (m_graph != nullptr) {
                m_graph->setTickLabelPosition(TickLabelPosition::OUTSIDE);
                m_graph->setXAxisTitle("TIME (5m)");
                m_graph->setYAxisTitle("YIELD (%)");
                m_graph->drawBackground();
                m_graph->drawData();
            }
            break;

        case STAGE_COMPACT:
            Serial.println("[V05DemoApp] Transitioning to STAGE_COMPACT");
            if (m_graph != nullptr) {
                m_graph->setTickLabelPosition(TickLabelPosition::INSIDE);
                m_graph->setXAxisTitle(nullptr);
                m_graph->setYAxisTitle(nullptr);
                m_graph->drawBackground();
                m_graph->drawData();
            }
            break;

        case STAGE_FINISHED:
            Serial.println("[V05DemoApp] Demo cycle FINISHED");
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
