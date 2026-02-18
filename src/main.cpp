/**
 * @file main.cpp
 * @brief LPad v0.75 Entry Point
 *
 * UIRenderManager-driven architecture with Widget-based System Menu,
 * background battery metering, power state management, and boot logo animation.
 *
 * Components:
 *   Z=0  PowerManager         (SystemComponent, battery polling + power states)
 *   Z=1  StockTickerApp       (AppComponent, main app)
 *   Z=5  BootLogoApp          (AppComponent, initial boot animation)
 *   Z=10 MiniLogoComponent    (SystemComponent, passive overlay)
 *   Z=20 SystemMenuComponent  (SystemComponent, activation=EDGE_DRAG TOP)
 */

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "apps/boot_logo_app.h"
#include "apps/stock_ticker_app.h"
#include "system/mini_logo_component.h"
#include "system/system_menu_component.h"
#include "system/power_manager.h"
#include "ui/ui_render_manager.h"
#include "theme_manager.h"
#include "relative_display.h"
#include "animation_ticker.h"
#include "input/touch_gesture_engine.h"
#include "wifi_config_generated.h"

#include "../hal/display.h"
#include "../hal/touch.h"
#include "../hal/network.h"

// --- Static globals ---
static AnimationTicker* g_ticker = nullptr;
static RelativeDisplay* g_relativeDisplay = nullptr;
static TouchGestureEngine* g_gestureEngine = nullptr;

static BootLogoApp* g_bootLogo = nullptr;
static StockTickerApp* g_stockTicker = nullptr;
static MiniLogoComponent* g_miniLogo = nullptr;
static SystemMenuComponent* g_systemMenu = nullptr;
static PowerManager* g_powerManager = nullptr;

static void displayError(const char* message) {
    hal_display_clear(LPad::ThemeManager::getInstance().getTheme()->colors.text_error);
    hal_display_flush();
    Serial.println("=== ERROR ===");
    Serial.println(message);
    Serial.println("=============");
}

// Background WiFi task — runs on Core 0 (where WiFi stack lives).
// Tries each configured network, then signals boot logo on success/failure.
static void wifiTaskFunction(void* param) {
    BootLogoApp* bootLogo = static_cast<BootLogoApp*>(param);

    Serial.println("[6/6] WiFi task started (background)...");
    Serial.printf("  [INFO] %d WiFi networks configured\n", g_wifi_count);

    bool connected = false;
    for (int i = 0; i < g_wifi_count; i++) {
        const char* ssid = g_wifi_config[i].ssid;
        Serial.printf("  [INFO] Trying %d/%d: %s ...\n", i + 1, g_wifi_count, ssid);
        Serial.flush();

        if (!hal_network_init(ssid, g_wifi_config[i].password)) {
            Serial.printf("  [FAIL] Init failed for %s\n", ssid);
            continue;
        }

        // Poll until connected, failed, or timeout (HAL handles 10s timeout internally)
        hal_network_status_t status = HAL_NETWORK_STATUS_CONNECTING;
        while (status == HAL_NETWORK_STATUS_CONNECTING) {
            vTaskDelay(pdMS_TO_TICKS(250));
            status = hal_network_get_status();
        }

        if (status == HAL_NETWORK_STATUS_CONNECTED) {
            Serial.printf("  [PASS] Connected to %s\n", ssid);
            connected = true;
            break;
        }

        Serial.printf("  [FAIL] Could not connect to %s\n", ssid);
    }

    if (connected) {
        bootLogo->setBootComplete();
        Serial.println("  [INFO] WiFi connected — boot complete signaled");
    } else {
        bootLogo->setErrorMessage("No WiFi Network Found");
        Serial.println("  [WARN] All WiFi networks failed — error displayed");
    }

    vTaskDelete(nullptr);  // One-shot task, self-delete
}

void setup() {
    Serial.begin(115200);

    // Deep sleep wakeup gate: on boards using deep sleep for shutdown
    // (T-Display S3 Plus), verify intentional startup via sustained button
    // hold. Returns immediately on cold boot or hardware-PEK boards.
    // Re-enters deep sleep if the hold is too short (does not return).
    hal_power_check_wakeup();

    delay(500);
    yield();

    Serial.println("\n\n\n=== LPad v0.75 (Power States) ===");
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

    // [3/6] RelativeDisplay + AnimationTicker + TouchGestureEngine
    Serial.println("[3/6] Creating display abstraction and timing...");
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

    // [4/6] Create standalone UI components + Power Manager
    Serial.println("[4/6] Creating UI components...");
    Serial.flush();

    const LPad::Theme* theme = LPad::ThemeManager::getInstance().getTheme();

    // Power Manager (Z=0) — created early so it can be registered with UIRenderManager
    g_powerManager = new PowerManager();
    if (!g_powerManager->begin()) {
        Serial.println("  [WARN] Power monitoring unavailable");
    } else {
        Serial.println("  [PASS] Power HAL initialized");
    }

    // Stock Ticker (Z=1)
    g_stockTicker = new StockTickerApp();
    if (!g_stockTicker->begin(g_relativeDisplay)) {
        displayError("StockTickerApp init failed");
        while (1) delay(1000);
    }

    // Mini Logo (Z=10) - hidden during boot animation, shown by BootLogoApp
    g_miniLogo = new MiniLogoComponent();
    if (!g_miniLogo->begin(g_relativeDisplay)) {
        displayError("MiniLogoComponent init failed");
        while (1) delay(1000);
    }

    // Boot Logo (Z=5) - plays once on initial boot, transitions to StockTicker
    g_bootLogo = new BootLogoApp();
    if (!g_bootLogo->begin(g_relativeDisplay, g_stockTicker, g_miniLogo)) {
        displayError("BootLogoApp init failed");
        while (1) delay(1000);
    }

    // System Menu (Z=20) - Widget-based with battery display
    g_systemMenu = new SystemMenuComponent();
    if (!g_systemMenu->begin(gfx, width, height)) {
        displayError("SystemMenuComponent init failed");
        while (1) delay(1000);
    }
    g_systemMenu->setVersion("Version 0.75");
    g_systemMenu->setSSIDProvider(hal_network_get_ssid);
    g_systemMenu->setSSID(hal_network_get_ssid());
    g_systemMenu->setBackgroundColor(theme->colors.system_menu_background);
    g_systemMenu->setRevealColor(theme->colors.background);
    g_systemMenu->setVersionFont(theme->fonts.smallest);
    g_systemMenu->setVersionColor(theme->colors.text_version);
    g_systemMenu->setSSIDFont(theme->fonts.normal);
    g_systemMenu->setSSIDColor(theme->colors.text_status);

    // Battery status provider
    g_systemMenu->setBatteryStatus(g_powerManager->getBatteryStatus());

    // Widget configuration (colors per ui_system_menu.md §2)
    g_systemMenu->setHeadingFont(theme->fonts.normal);          // 12pt per spec
    g_systemMenu->setHeadingColor(theme->colors.text_heading);  // Khaki
    g_systemMenu->setHeadingUnderlined(true);
    g_systemMenu->setListFont(theme->fonts.normal);
    g_systemMenu->setWidgetColors(
        theme->colors.text_main,        // normalText (Khaki per spec)
        theme->colors.text_highlight,   // highlight (connected/Chamoisee)
        theme->colors.bg_connecting,    // connectingBg
        theme->colors.text_error,       // errorText (failed)
        theme->colors.scroll_indicator  // scrollIndicator
    );

    // Populate WiFi list from compiled config
    if (g_wifi_count > 0) {
        g_systemMenu->setWiFiEntries(g_wifi_config, g_wifi_count);
        Serial.printf("  [INFO] WiFi list populated with %d networks\n", g_wifi_count);
    }

    Serial.println("  [PASS] BootLogo + StockTicker + MiniLogo + SystemMenu(Widgets) created");
    yield();

    // [5/6] Register with UIRenderManager + start boot logo
    Serial.println("[5/6] Registering with UIRenderManager...");
    Serial.flush();

    auto& mgr = UIRenderManager::getInstance();
    mgr.reset();
    mgr.setFlushCallback(hal_display_flush);

    mgr.registerComponent(g_powerManager, 0);
    mgr.registerComponent(g_stockTicker, 1);
    mgr.registerComponent(g_bootLogo, 5);
    mgr.registerComponent(g_miniLogo, 10);

    g_miniLogo->hide(); // Hidden during boot animation, shown by BootLogoApp
    g_systemMenu->setActivationEvent(TOUCH_EDGE_DRAG, TOUCH_DIR_UP);
    g_systemMenu->hide(); // Start hidden
    mgr.registerComponent(g_systemMenu, 20);

    // Boot sequence: BootLogoApp plays first, then transitions to StockTicker
    mgr.setActiveApp(g_bootLogo);

    // Clear display with theme background
    hal_display_clear(theme->colors.background);
    hal_display_flush();

    Serial.println("  [PASS] UIRenderManager configured — boot logo active");
    yield();

    // [6/6] Launch background WiFi task (Core 0, where WiFi stack runs)
    Serial.println("[6/6] Launching background WiFi task...");
    Serial.flush();

    BaseType_t result = xTaskCreatePinnedToCore(
        wifiTaskFunction,
        "wifi_boot",
        8192,        // 8KB stack — no TLS, just WiFi connect
        g_bootLogo,  // Task parameter
        1,           // Priority
        nullptr,     // No handle needed (one-shot, self-deleting)
        0            // Core 0 (WiFi stack core)
    );

    if (result != pdPASS) {
        Serial.println("  [FAIL] WiFi task creation failed");
        g_bootLogo->setErrorMessage("WiFi Task Failed");
    } else {
        Serial.println("  [PASS] WiFi task launched on Core 0");
    }

    Serial.println("\n=== LPad v0.75 Started ===");
    Serial.printf("    Components: %d\n", mgr.getComponentCount());
    Serial.println("    Z=0:  PowerManager (System, background polling)");
    Serial.println("    Z=1:  StockTicker  (App, main — activated after boot logo)");
    Serial.println("    Z=5:  BootLogo     (App, initial boot animation)");
    Serial.println("    Z=10: MiniLogo     (System, shown after boot animation)");
    Serial.println("    Z=20: SystemMenu   (System, activation=EDGE_DRAG TOP, Widget-based)");
    Serial.println("Swipe down from top edge to open System Menu");
    Serial.println("Short press power button to suspend/resume");
    Serial.println("Long press (4s) power button to shutdown");
    Serial.flush();
}

void loop() {
    float deltaTime = g_ticker->waitForNextFrame();

    // --- Power button polling (suspend/resume/shutdown) ---
    // Must run before render pipeline. If suspend is triggered,
    // handle() blocks until resume completes.
    g_powerManager->handle();

    // --- Serial screenshot trigger ---
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 'S') {
            hal_display_dump_screen();
        }
    }

    // --- Touch input -> gesture -> UIRenderManager ---
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
