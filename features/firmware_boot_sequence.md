# Feature: Firmware Boot Sequence

> Label: "Firmware: Boot Sequence"
> Category: "Firmware Lifecycle"
> Prerequisite: features/arch_component_model.md
> Prerequisite: features/arch_concurrency.md
> Prerequisite: features/arch_hal_contract.md

[TODO]

## 1. Overview

The firmware boot sequence is the `setup()` entry point in `main.cpp` that initializes all hardware, creates the UI component stack, registers components with the UIRenderManager, and launches a background WiFi task. It executes as a strict 6-stage pipeline where each stage depends on the previous stage's success. Fatal initialization failures halt the system with a visible error screen. The `loop()` function implements the main 30 FPS frame loop: power management, serial debug input, touch gesture processing, and Z-ordered rendering.

---

## 2. Requirements

### 2.1 Pre-Init

- Serial MUST be initialized at 115200 baud.
- `hal_power_check_wakeup()` MUST be called before any other initialization to gate deep-sleep wakeup on boards that use it (T-Display S3 Plus). Returns immediately on cold boot or hardware-PEK boards.
- A 500ms delay and `yield()` MUST follow to allow serial monitor attachment and watchdog servicing.

### 2.2 Stage 1: Display HAL

- `hal_display_init()` MUST be called. Failure MUST halt with a solid error-colored screen and serial error message.
- `APP_DISPLAY_ROTATION` preprocessor define, if set, MUST be applied via `hal_display_set_rotation()`.
- Display dimensions (width, height) MUST be retrieved and logged.

### 2.3 Stage 2: Touch HAL

- `hal_touch_init()` MUST be called. Failure MUST halt with error screen.
- Touch MUST be initialized before the gesture engine (Stage 3).

### 2.4 Stage 3: Display Abstraction and Timing

- `display_relative_init()` MUST be called to initialize the coordinate transformation system.
- The GFX object MUST be retrieved via `hal_display_get_gfx()`. Null return MUST halt with error screen.
- `RelativeDisplay` MUST be constructed with the GFX object and display dimensions, then initialized.
- `AnimationTicker` MUST be constructed with a 30 FPS target.
- `TouchGestureEngine` MUST be constructed with display dimensions, then configured with board-specific touch characteristics via `hal_touch_configure_gesture_engine()`.

### 2.5 Stage 4: UI Component Creation

- Components MUST be created in this order with these Z-layers:
  - **PowerManager** (Z=0): `begin()` failure is a WARNING, not fatal (power monitoring unavailable).
  - **StockTickerApp** (Z=1): `begin(relativeDisplay)`. Failure MUST halt.
  - **MiniLogoComponent** (Z=10): `begin(relativeDisplay)`. Failure MUST halt. Hidden initially.
  - **BootLogoApp** (Z=5): `begin(relativeDisplay, stockTicker, miniLogo)`. Failure MUST halt.
  - **SystemMenuComponent** (Z=20): `begin(gfx, width, height)`. Failure MUST halt. Configured with version string, SSID provider, battery status, theme colors, theme fonts, heading style, widget colors, and WiFi entries from compiled config.

### 2.6 Stage 5: UIRenderManager Registration

- `UIRenderManager::getInstance()` MUST be reset and configured with `hal_display_flush` as the flush callback.
- Components MUST be registered in Z-order: 0, 1, 5, 10, 20.
- MiniLogoComponent MUST start hidden.
- SystemMenuComponent MUST be configured with activation event `TOUCH_EDGE_DRAG` direction `TOUCH_DIR_UP`, and MUST start hidden.
- BootLogoApp MUST be set as the active app.
- Display MUST be cleared with theme background color and flushed.

### 2.7 Stage 6: Background WiFi Task

- A FreeRTOS task MUST be created pinned to Core 0 with 8KB stack, priority 1.
- The task receives the BootLogoApp pointer as its parameter.
- Task function iterates compiled WiFi networks: for each, calls `hal_network_init(ssid, password)`, polls `hal_network_get_status()` every 250ms until connected, failed, or timeout.
- On first successful connection: calls `bootLogo->setBootComplete()` (cross-core signal).
- If all networks fail: calls `bootLogo->setErrorMessage("No WiFi Network Found")`.
- Task self-deletes via `vTaskDelete(nullptr)` after completion (one-shot).
- If `xTaskCreatePinnedToCore` fails: error message set on boot logo immediately.

### 2.8 Main Loop

- Frame timing: `AnimationTicker::waitForNextFrame()` blocks until next 30 FPS frame, returns delta time in seconds.
- Power management: `PowerManager::handle()` polls power button. Blocks during suspend until resume.
- Serial debug: reads 'S' character to trigger `hal_display_dump_screen()` (screenshot).
- Touch pipeline: reads raw touch via HAL, detects home button (CST816 virtual button maps to EDGE_DRAG DOWN), feeds gesture coordinates to `TouchGestureEngine::update()`, routes detected gestures to `UIRenderManager::routeInput()`.
- Rendering: `UIRenderManager::renderAll()` draws all visible components in Z-order. `UIRenderManager::updateAll(deltaTime)` advances animations.

### 2.9 Error Handling

- Fatal initialization errors MUST fill the display with `theme.colors.text_error` color, print to serial, and enter infinite delay loop.
- Each stage MUST log pass/fail status to serial with stage number prefix (`[N/6]`).
- Startup summary MUST list all registered components with their Z-order and roles.

---

## 3. Scenarios

> **[Draft]** These scenarios were auto-generated from existing code by /pl-spec-from-code. Review and refine before marking as final.

### Automated Scenarios

#### Scenario: Display init failure halts boot

    Given hal_display_init returns false
    When setup runs Stage 1
    Then the display is filled with error color
    And the system enters an infinite loop

#### Scenario: Touch init failure halts boot

    Given hal_display_init succeeds
    And hal_touch_init returns false
    When setup runs Stage 2
    Then the display shows an error screen
    And the system enters an infinite loop

#### Scenario: PowerManager failure is non-fatal

    Given all HAL init calls succeed
    When PowerManager begin returns false
    Then a warning is logged
    And setup continues to Stage 5

#### Scenario: Components registered in correct Z-order

    Given all components are created successfully
    When UIRenderManager registration completes
    Then component count is 5
    And Z-order is PowerManager(0), StockTicker(1), BootLogo(5), MiniLogo(10), SystemMenu(20)

#### Scenario: WiFi task signals boot complete on connection

    Given the WiFi task is running on Core 0
    And hal_network_init succeeds for a configured network
    When hal_network_get_status returns HAL_NETWORK_STATUS_CONNECTED
    Then setBootComplete is called on the BootLogoApp
    And the task self-deletes

#### Scenario: WiFi task signals error when all networks fail

    Given the WiFi task is running on Core 0
    And all configured networks fail to connect
    When the last network attempt completes
    Then setErrorMessage is called with "No WiFi Network Found"
    And the task self-deletes

#### Scenario: WiFi task creation failure sets error on boot logo

    Given xTaskCreatePinnedToCore returns a value other than pdPASS
    When Stage 6 completes
    Then setErrorMessage is called with "WiFi Task Failed"

#### Scenario: Main loop runs at 30 FPS

    Given setup has completed successfully
    When loop executes
    Then waitForNextFrame blocks until the next 30fps interval
    And returns a delta time in seconds

### Manual Scenarios (Human Verification Required)

#### Scenario: Full boot sequence completes on hardware

    Given the firmware is uploaded to a supported board
    When the device is powered on
    Then the boot logo animation plays
    And WiFi connects in the background
    And the stock chart appears after connection
    And the system menu is accessible via top-edge swipe
