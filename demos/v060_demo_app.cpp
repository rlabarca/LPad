/**
 * @file v060_demo_app.cpp
 * @brief Release 0.60 Demo Application Implementation
 */

#include "v060_demo_app.h"
#include "../hal/display.h"
#include "../src/theme_manager.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

V060DemoApp::V060DemoApp()
    : m_currentPhase(PHASE_LOGO)
    , m_display(nullptr)
    , m_logoScreen(nullptr)
    , m_connectivityScreen(nullptr)
    , m_graph(nullptr)
    , m_miniLogo(nullptr)
    , m_stockTracker(nullptr)
    , m_titleBuffer(nullptr)
    , m_titleBufferX(0)
    , m_titleBufferY(0)
    , m_titleBufferWidth(0)
    , m_titleBufferHeight(0)
    , m_titleBufferValid(false)
    , m_logoAnimationComplete(false)
    , m_pingResult(false)
    , m_graphInitialRenderDone(false)
    , m_logoHoldTimer(0.0f)
    , m_handoverTimer(0.0f)
{
}

V060DemoApp::~V060DemoApp() {
    // Stop stock tracker before cleanup
    if (m_stockTracker != nullptr) {
        m_stockTracker->stop();
    }

    delete m_logoScreen;
    delete m_connectivityScreen;
    delete m_graph;
    delete m_miniLogo;
    delete m_stockTracker;

    if (m_titleBuffer != nullptr) {
        free(m_titleBuffer);
    }
}

bool V060DemoApp::begin(RelativeDisplay* display) {
    if (display == nullptr) {
        Serial.println("[V060DemoApp] ERROR: display is nullptr");
        return false;
    }

    m_display = display;

    // Get theme colors
    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();

    // Clear display immediately
    Serial.println("[V060DemoApp] Clearing display at startup...");
    Arduino_GFX* gfx = m_display->getGfx();
    if (gfx != nullptr) {
        gfx->fillScreen(theme->colors.background);
        hal_display_flush();
    }

    // Create LogoScreen with theme background
    m_logoScreen = new LogoScreen();
    if (!m_logoScreen->begin(m_display, theme->colors.background)) {
        Serial.println("[V060DemoApp] ERROR: LogoScreen initialization failed");
        return false;
    }

    // Create ConnectivityStatusScreen
    m_connectivityScreen = new ConnectivityStatusScreen();
    if (!m_connectivityScreen->begin(m_display)) {
        Serial.println("[V060DemoApp] ERROR: ConnectivityStatusScreen initialization failed");
        return false;
    }

    // Create MiniLogo (top-right corner)
    m_miniLogo = new MiniLogo(m_display, MiniLogo::Corner::TOP_RIGHT);

    Serial.println("[V060DemoApp] Initialized successfully - starting in LOGO phase");
    return true;
}

void V060DemoApp::update(float deltaTime) {
    switch (m_currentPhase) {
        case PHASE_LOGO: {
            // Update logo screen
            if (m_logoScreen != nullptr) {
                m_logoScreen->update(deltaTime);

                // Check if animation is complete
                if (!m_logoAnimationComplete && m_logoScreen->isDone()) {
                    m_logoAnimationComplete = true;
                    m_logoHoldTimer = 0.0f;
                    Serial.println("[V060DemoApp] Logo animation complete, starting hold timer");
                }

                // Hold at final position for LOGO_HOLD_DURATION
                if (m_logoAnimationComplete) {
                    m_logoHoldTimer += deltaTime;
                    if (m_logoHoldTimer >= LOGO_HOLD_DURATION) {
                        Serial.println("[V060DemoApp] Logo hold complete, transitioning to CONNECTIVITY");
                        transitionToPhase(PHASE_CONNECTIVITY);
                    }
                }
            }
            break;
        }

        case PHASE_CONNECTIVITY: {
            // Check network status
            hal_network_status_t status = hal_network_get_status();

            // Once connected, perform ping test
            if (status == HAL_NETWORK_STATUS_CONNECTED && !m_pingResult) {
                Serial.println("[V060DemoApp] Wi-Fi connected, performing ping test...");
                m_pingResult = hal_network_ping("8.8.8.8");
                if (m_pingResult) {
                    Serial.println("[V060DemoApp] Ping test successful!");
                    transitionToPhase(PHASE_HANDOVER);
                    return;
                } else {
                    Serial.println("[V060DemoApp] Ping test failed");
                }
            }

            // Update connectivity screen
            if (m_connectivityScreen != nullptr) {
                m_connectivityScreen->update(m_pingResult);

                // Re-render mini logo on top (connectivity screen clears the screen when updating)
                if (m_miniLogo != nullptr) {
                    m_miniLogo->render();
                }

                hal_display_flush();
            }
            break;
        }

        case PHASE_HANDOVER: {
            // Hold "PING OK" for 2 seconds
            m_handoverTimer += deltaTime;
            if (m_handoverTimer >= HANDOVER_DURATION) {
                transitionToPhase(PHASE_STOCK_GRAPH);
                return;
            }

            // Continue updating connectivity screen to show "PING OK"
            if (m_connectivityScreen != nullptr) {
                m_connectivityScreen->update(m_pingResult);

                // Re-render mini logo on top (connectivity screen clears the screen when updating)
                if (m_miniLogo != nullptr) {
                    m_miniLogo->render();
                }

                hal_display_flush();
            }
            break;
        }

        case PHASE_STOCK_GRAPH: {
            // Graph rendering happens in render()
            // StockTracker runs autonomously in background task
            break;
        }
    }
}

void V060DemoApp::render() {
    switch (m_currentPhase) {
        case PHASE_LOGO:
            // Logo screen handles its own rendering via update()
            break;

        case PHASE_CONNECTIVITY:
        case PHASE_HANDOVER:
            // Connectivity screen handles its own rendering via update()
            // Mini logo is rendered by connectivity screen's update() when it clears the screen
            break;

        case PHASE_STOCK_GRAPH: {
            bool needsFullRender = !m_graphInitialRenderDone;
            static bool backgroundDrawn = false;

            // Update graph data if available
            if (m_graph != nullptr && m_stockTracker != nullptr) {
                DataItemTimeSeries* dataSeries = m_stockTracker->getDataSeries();

                if (dataSeries != nullptr && dataSeries->getLength() > 0) {
                    // Check if data has been updated since last render
                    static size_t lastDataLength = 0;
                    size_t currentDataLength = dataSeries->getLength();

                    if (currentDataLength != lastDataLength) {
                        // Data changed - update graph and trigger full render
                        GraphData graphData = dataSeries->getGraphData();
                        m_graph->setData(graphData);

                        // Draw background (axes + ticks) after first data load (needed for tick calculation)
                        if (!backgroundDrawn) {
                            m_graph->drawBackground();
                            backgroundDrawn = true;
                            Serial.println("[V060DemoApp] Background (axes + ticks) drawn with data-based tick marks");
                        }

                        m_graph->drawData();
                        lastDataLength = currentDataLength;
                        needsFullRender = true;

                        Serial.printf("[V060DemoApp] Graph data updated: %zu points\n", currentDataLength);
                    }
                }

                if (needsFullRender) {
                    // Full render: composite all graph layers
                    m_graph->render();

                    // Render mini logo in top-right corner (once, not every frame)
                    if (m_miniLogo != nullptr) {
                        m_miniLogo->render();
                    }

                    // Render "DEMO v0.60" title in top-left corner (once, not every frame)
                    blitTitle();

                    // Flush to display
                    hal_display_flush();

                    m_graphInitialRenderDone = true;
                }

                // Always update live indicator animation (efficient dirty-rect update)
                m_graph->update(0.033f);  // ~30fps delta time
            }
            break;
        }
    }
}

void V060DemoApp::transitionToPhase(Phase newPhase) {
    m_currentPhase = newPhase;
    m_handoverTimer = 0.0f;

    switch (newPhase) {
        case PHASE_LOGO:
            Serial.println("[V060DemoApp] Transitioning to PHASE_LOGO");
            m_logoAnimationComplete = false;
            m_logoHoldTimer = 0.0f;
            break;

        case PHASE_CONNECTIVITY:
            Serial.println("[V060DemoApp] Transitioning to PHASE_CONNECTIVITY");
            m_pingResult = false;

            // Start Wi-Fi connection process
            #ifdef LPAD_WIFI_SSID
            if (hal_network_init(LPAD_WIFI_SSID, LPAD_WIFI_PASSWORD)) {
                Serial.printf("[V060DemoApp] Connecting to Wi-Fi: %s\n", LPAD_WIFI_SSID);
            } else {
                Serial.println("[V060DemoApp] Network initialization failed");
            }
            #else
            Serial.println("[V060DemoApp] No Wi-Fi credentials configured");
            #endif
            break;

        case PHASE_HANDOVER:
            Serial.println("[V060DemoApp] Transitioning to PHASE_HANDOVER (holding PING OK)");
            break;

        case PHASE_STOCK_GRAPH:
            Serial.println("[V060DemoApp] Transitioning to PHASE_STOCK_GRAPH");

            // Create stock tracker for ^TNX (60 second refresh, 30 minute history)
            Serial.println("[V060DemoApp] Creating StockTracker for ^TNX...");
            m_stockTracker = new StockTracker("^TNX", 60, 30);
            if (!m_stockTracker->start()) {
                Serial.println("[V060DemoApp] ERROR: Failed to start StockTracker");
            } else {
                Serial.println("[V060DemoApp] StockTracker started successfully");
            }

            // Create graph with v2 styling
            Serial.println("[V060DemoApp] Creating TimeSeriesGraph with v2 styling...");
            GraphTheme theme = createStockGraphTheme();

            if (m_display != nullptr) {
                Arduino_GFX* gfx = m_display->getGfx();
                if (gfx != nullptr) {
                    int32_t width = hal_display_get_width_pixels();
                    int32_t height = hal_display_get_height_pixels();

                    m_graph = new TimeSeriesGraph(theme, gfx, width, height);

                    if (!m_graph->begin()) {
                        Serial.println("[V060DemoApp] ERROR: TimeSeriesGraph initialization failed");
                    } else {
                        // Configure graph for v2 styling
                        m_graph->setTickLabelPosition(TickLabelPosition::INSIDE);
                        m_graph->setYAxisTitle("Value");
                        m_graph->setXAxisTitle("Mins Prior");
                        m_graph->setYTicks(0.002f);  // Set Y-axis tick spacing
                        // NOTE: drawBackground() will be called after first data load (needs data for tick calculation)
                        Serial.println("[V060DemoApp] TimeSeriesGraph initialized with INSIDE labels and axis titles");
                    }
                }
            }

            // Render title to buffer for fast blitting
            renderTitleToBuffer();

            break;
    }
}

GraphTheme V060DemoApp::createStockGraphTheme() {
    GraphTheme theme = {};

    // Get theme colors from ThemeManager
    const LPad::Theme* lpadTheme = LPad::ThemeManager::getInstance().getTheme();

    // Use theme background color
    theme.backgroundColor = lpadTheme->colors.background;
    theme.useBackgroundGradient = false;

    // Solid white line
    theme.lineColor = RGB565_WHITE;
    theme.useLineGradient = false;

    // Use theme secondary color for axes
    theme.axisColor = lpadTheme->colors.secondary;

    // Line and axis styling
    theme.lineThickness = 2.0f;
    theme.axisThickness = 0.8f;
    theme.tickColor = lpadTheme->colors.graph_ticks;
    theme.tickLength = 5.0f;  // Increased from 2.5 for better visibility

    // Live indicator: use theme accent color instead of hardcoded green
    theme.liveIndicatorGradient.color_stops[0] = lpadTheme->colors.accent;
    theme.liveIndicatorGradient.color_stops[1] = lpadTheme->colors.accent;  // Same color = solid
    theme.liveIndicatorPulseSpeed = 0.5f;

    // Font assignments from ThemeManager
    theme.tickFont = lpadTheme->fonts.smallest;
    theme.axisTitleFont = lpadTheme->fonts.ui;

    return theme;
}

void V060DemoApp::renderTitleToBuffer() {
    if (m_display == nullptr) {
        return;
    }

    Arduino_GFX* gfx = m_display->getGfx();
    if (gfx == nullptr) {
        return;
    }

    const char* titleText = "DEMO v0.60";

    // Get theme font (using smallest font for compact display)
    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();
    const GFXfont* font = theme->fonts.smallest;

    if (font == nullptr) {
        Serial.println("[V060DemoApp] ERROR: font is nullptr");
        return;
    }

    // Set font temporarily to measure bounds
    gfx->setFont(font);

    int16_t x1, y1;
    uint16_t w, h;
    gfx->getTextBounds(titleText, 0, 0, &x1, &y1, &w, &h);

    // Position in top-left corner with minimal offset
    m_titleBufferX = 0;
    m_titleBufferY = 0 - y1;  // Adjust for baseline
    m_titleBufferWidth = w;
    m_titleBufferHeight = h;

    // Allocate buffer
    size_t bufferSize = m_titleBufferWidth * m_titleBufferHeight * sizeof(uint16_t);
    m_titleBuffer = (uint16_t*)malloc(bufferSize);

    if (m_titleBuffer == nullptr) {
        Serial.println("[V060DemoApp] ERROR: Failed to allocate title buffer");
        return;
    }

    // Create temporary canvas for title rendering
    Arduino_GFX* mainGfx = m_display->getGfx();
    Arduino_Canvas* canvas = new Arduino_Canvas(m_titleBufferWidth, m_titleBufferHeight, mainGfx);
    if (canvas == nullptr || !canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) {
        Serial.println("[V060DemoApp] ERROR: Failed to create title canvas");
        free(m_titleBuffer);
        m_titleBuffer = nullptr;
        delete canvas;
        return;
    }

    // Get canvas buffer pointer for fast blitting
    uint16_t* canvasBuffer = canvas->getFramebuffer();
    if (canvasBuffer == nullptr) {
        Serial.println("[V060DemoApp] ERROR: Canvas framebuffer is nullptr");
        delete canvas;
        free(m_titleBuffer);
        m_titleBuffer = nullptr;
        return;
    }

    // Render title to canvas with chroma key for transparency
    constexpr uint16_t CHROMA_KEY = 0x0001;
    canvas->fillScreen(CHROMA_KEY);  // Chroma key background for transparency
    canvas->setFont(font);
    canvas->setTextColor(RGB565_WHITE);
    canvas->setCursor(0, -y1);  // Position at top-left of canvas
    canvas->print(titleText);

    // Copy canvas buffer to title buffer
    memcpy(m_titleBuffer, canvasBuffer, bufferSize);

    delete canvas;

    m_titleBufferValid = true;
    Serial.printf("[V060DemoApp] Title buffer created: %dx%d at (%d, %d)\n",
                  m_titleBufferWidth, m_titleBufferHeight, m_titleBufferX, m_titleBufferY);
}

void V060DemoApp::blitTitle() {
    if (!m_titleBufferValid || m_titleBuffer == nullptr) {
        return;
    }

    // Fast blit title buffer to display with transparency (chroma key 0x0001)
    constexpr uint16_t CHROMA_KEY = 0x0001;
    hal_display_fast_blit_transparent(m_titleBufferX, m_titleBufferY,
                                      m_titleBufferWidth, m_titleBufferHeight,
                                      m_titleBuffer, CHROMA_KEY);
}
