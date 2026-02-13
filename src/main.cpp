/**
 * @file main.cpp
 * @brief LPad v0.70+ Entry Point
 *
 * Clean entry point for the UIRenderManager-driven architecture.
 * No dispatcher, no #ifdef, no demo includes.
 *
 * Components:
 *   Z=1  StockTickerApp   (AppComponent)
 *   Z=10 MiniLogoComponent (SystemComponent, passive overlay)
 *   Z=20 SystemMenuComponent (SystemComponent, activation=EDGE_DRAG TOP)
 */

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "apps/stock_ticker_app.h"
#include "system/mini_logo_component.h"
#include "system/system_menu_component.h"
#include "ui/ui_render_manager.h"
#include "theme_manager.h"
#include "relative_display.h"
#include "animation_ticker.h"
#include "input/touch_gesture_engine.h"

#include "../hal/display.h"
#include "../hal/touch.h"
#include "../hal/network.h"

// --- Static globals ---
static AnimationTicker* g_ticker = nullptr;
static RelativeDisplay* g_relativeDisplay = nullptr;
static TouchGestureEngine* g_gestureEngine = nullptr;

static StockTickerApp* g_stockTicker = nullptr;
static MiniLogoComponent* g_miniLogo = nullptr;
static SystemMenuComponent* g_systemMenu = nullptr;

static void displayError(const char* message) {
    hal_display_clear(LPad::ThemeManager::getInstance().getTheme()->colors.text_error);
    hal_display_flush();
    Serial.println("=== ERROR ===");
    Serial.println(message);
    Serial.println("=============");
}

void setup() {
    Serial.begin(115200);
    delay(500);
    yield();

    Serial.println("\n\n\n=== LPad v0.70 (Standalone Components) ===");
    Serial.flush();
    yield();

    // [1/6] Display HAL
    Serial.println("[1/6] Initializing display HAL...");
    Serial.flush();

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
    yield();

    // [2/6] Touch HAL
    Serial.println("[2/6] Initializing touch HAL...");
    Serial.flush();

    if (!hal_touch_init()) {
        displayError("Touch initialization failed");
        while (1) delay(1000);
    }
    Serial.println("  [PASS] Touch initialized");
    yield();

    // [3/6] WiFi
    Serial.println("[3/6] Initializing WiFi...");
    Serial.flush();

    #ifdef LPAD_WIFI_SSID
    if (hal_network_init(LPAD_WIFI_SSID, LPAD_WIFI_PASSWORD)) {
        Serial.printf("  [INFO] Connecting to: %s\n", LPAD_WIFI_SSID);
    } else {
        Serial.println("  [WARN] Network init failed");
    }
    #else
    Serial.println("  [INFO] No WiFi credentials configured");
    #endif
    yield();

    // [4/6] RelativeDisplay + AnimationTicker + TouchGestureEngine
    Serial.println("[4/6] Creating display abstraction and timing...");
    Serial.flush();

    display_relative_init();
    Arduino_GFX* gfx = static_cast<Arduino_GFX*>(hal_display_get_gfx());
    if (gfx == nullptr) {
        displayError("Display object unavailable");
        while (1) delay(1000);
    }

    static RelativeDisplay relDisplay(gfx, width, height);
    g_relativeDisplay = &relDisplay;
    g_relativeDisplay->init();

    static AnimationTicker ticker(30);
    g_ticker = &ticker;

    g_gestureEngine = new TouchGestureEngine(
        static_cast<int16_t>(width),
        static_cast<int16_t>(height)
    );
    hal_touch_configure_gesture_engine(g_gestureEngine);

    Serial.println("  [PASS] RelativeDisplay + 30fps Ticker + GestureEngine");
    yield();

    // [5/6] Create standalone components
    Serial.println("[5/6] Creating UI components...");
    Serial.flush();

    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();

    // Stock Ticker (Z=1)
    g_stockTicker = new StockTickerApp();
    if (!g_stockTicker->begin(g_relativeDisplay)) {
        displayError("StockTickerApp init failed");
        while (1) delay(1000);
    }

    // Mini Logo (Z=10)
    g_miniLogo = new MiniLogoComponent();
    if (!g_miniLogo->begin(g_relativeDisplay)) {
        displayError("MiniLogoComponent init failed");
        while (1) delay(1000);
    }

    // System Menu (Z=20)
    g_systemMenu = new SystemMenuComponent();
    if (!g_systemMenu->begin(gfx, width, height)) {
        displayError("SystemMenuComponent init failed");
        while (1) delay(1000);
    }
    g_systemMenu->setVersion("Version 0.70");
    g_systemMenu->setSSIDProvider(hal_network_get_ssid);
    g_systemMenu->setSSID(hal_network_get_ssid());
    g_systemMenu->setBackgroundColor(theme->colors.system_menu_bg);
    g_systemMenu->setRevealColor(theme->colors.background);
    g_systemMenu->setVersionFont(theme->fonts.smallest);
    g_systemMenu->setVersionColor(theme->colors.text_version);
    g_systemMenu->setSSIDFont(theme->fonts.normal);
    g_systemMenu->setSSIDColor(theme->colors.text_status);

    Serial.println("  [PASS] StockTicker + MiniLogo + SystemMenu created");
    yield();

    // [6/6] Register with UIRenderManager
    Serial.println("[6/6] Registering with UIRenderManager...");
    Serial.flush();

    auto& mgr = UIRenderManager::getInstance();
    mgr.reset();
    mgr.setFlushCallback(hal_display_flush);

    mgr.registerComponent(g_stockTicker, 1);
    mgr.registerComponent(g_miniLogo, 10);

    g_systemMenu->setActivationEvent(TOUCH_EDGE_DRAG, TOUCH_DIR_UP);
    g_systemMenu->hide(); // Start hidden
    mgr.registerComponent(g_systemMenu, 20);

    mgr.setActiveApp(g_stockTicker);

    Serial.println("  [PASS] UIRenderManager configured:");
    Serial.printf("    Components: %d\n", mgr.getComponentCount());
    Serial.println("    Z=1:  StockTicker  (App)");
    Serial.println("    Z=10: MiniLogo     (System, always visible)");
    Serial.println("    Z=20: SystemMenu   (System, activation=EDGE_DRAG TOP)");

    // Clear display with theme background
    gfx->fillScreen(theme->colors.background);
    hal_display_flush();

    Serial.println("\n=== LPad v0.70 Started ===");
    Serial.println("Swipe up from bottom edge to open System Menu");
    Serial.flush();
}

void loop() {
    float deltaTime = g_ticker->waitForNextFrame();

    // --- Touch input → gesture → UIRenderManager ---
    hal_touch_point_t touch_point;
    bool touch_ok = hal_touch_read(&touch_point);

    if (touch_ok) {
        touch_gesture_event_t gesture_event;
        bool gesture_detected = false;

        if (touch_point.is_home_button) {
            gesture_event.type = TOUCH_EDGE_DRAG;
            gesture_event.direction = TOUCH_DIR_DOWN;
            gesture_event.x_px = static_cast<int16_t>(hal_display_get_width_pixels() / 2);
            gesture_event.y_px = static_cast<int16_t>(hal_display_get_height_pixels() - 1);
            gesture_event.x_percent = 0.5f;
            gesture_event.y_percent = 1.0f;
            gesture_detected = true;
        } else {
            uint32_t dt_ms = static_cast<uint32_t>(deltaTime * 1000.0f);
            gesture_detected = g_gestureEngine->update(
                touch_point.x, touch_point.y,
                touch_point.is_pressed, dt_ms,
                &gesture_event
            );
        }

        if (gesture_detected) {
            UIRenderManager::getInstance().routeInput(gesture_event);
        }
    }

    // --- Render (Painter's Algorithm) + flush ---
    UIRenderManager::getInstance().renderAll();

    // --- Update animations ---
    UIRenderManager::getInstance().updateAll(deltaTime);
}
