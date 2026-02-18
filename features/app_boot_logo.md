# Feature: App Component - Boot Logo

> Label: "App Component: Boot Logo"
> Category: "Applications"
> Prerequisite: features/core_ui_render_manager.md
> Prerequisite: features/ui_vector_assets.md
> Prerequisite: features/app_stock_ticker.md

## 1. Overview
This feature defines a dedicated `AppComponent` that displays an animation immediately upon power-on. It serves as a visual indicator that the device is booting, while core system services (e.g., Network, Touch) initialize in the background. It is also responsible for displaying any critical errors that occur during the boot process.

## 2. Configuration
*   **Type:** `AppComponent`
*   **Z-Order:** 5 (Above default App Z-Order, below System Components)
*   **Name:** "BootLogo"

## 3. Behavior

### 3.1 Parallel Initialization
*   The `BootLogo` app should be rendered as soon as the display HAL is initialized.
*   All other system initializations should proceed in the background (e.g., in a separate FreeRTOS task) so they do not block the boot animation from displaying.

### 3.2 Animation
*   The animation sequence (e.g., logo scaling and moving) should loop or hold its final frame indefinitely until it is either dismissed (on successful boot) or interrupted by an error.

### 3.3 Boot Status Display
*   **Default State:** While background initialization is in progress, the standard boot animation is displayed.
*   **Error State:**
    *   The app must provide an interface (e.g., a public method `setErrorMessage(const char* message)`) for the background initialization process to report a fatal error.
    *   Upon receiving an error message, the animation must stop.
    *   The provided error message must be rendered in the exact center of the screen.
    *   The text must use the standard font and color specified by the currently active theme.
    *   The boot screen, with the error message, should remain visible indefinitely, and the boot process should halt.

### 3.4 Dismissal
*   The `BootLogo` app is not responsible for dismissing itself.
*   An external boot manager (e.g., logic in `main.cpp`) will be responsible for closing the `BootLogo` app and transitioning to the main application (`StockTicker`) once all background initializations have completed successfully.

## 4. Scenarios

### Scenario: Successful Asynchronous Boot
    Given the device has just powered on
    And the display is initialized
    When the main boot process starts
    Then the "BootLogo" app is displayed immediately and starts its animation
    And core systems (Network, Touch, etc.) initialize in the background, in parallel
    And when all systems have initialized successfully
    Then the main boot process closes the "BootLogo" app
    And the "StockTicker" app begins running

### Scenario: Failed Boot (No WiFi)
    Given the "BootLogo" app is running during startup
    And the background WiFi initialization task fails to find a valid network
    When the error is reported to the boot screen
    Then the boot animation stops (or is hidden)
    And the friendly error message "No WiFi Network Found" is displayed in the center of the screen
    And the text is styled using the active theme's default font and color
    And the boot screen remains visible with the error message, halting the boot sequence

### Scenario: Resume from Suspend
    Given the "StockTicker" app is running
    And the device enters a suspended state (e.g., light sleep)
    When the device resumes
    Then the "StockTicker" app receives `onUnpause()` and continues running
    And the "BootLogo" app does NOT run again

## Implementation Notes

### [2026-02-16] App Chaining Pattern
The `BootLogoApp` is initialized with a pointer to the next app (`StockTickerApp`) and an optional `SystemComponent*` for the MiniLogo overlay. In `update()`, once the animation reaches DONE state, it calls `UIRenderManager::getInstance().setActiveApp(m_nextApp)` which pauses the boot logo and runs the stock ticker. The `UIRenderManager` handles the `onPause()`/`onRun()` lifecycle automatically.

### [2026-02-16] MiniLogo Visibility During Boot
The MiniLogo overlay (Z=10) starts hidden via `hide()` in `main.cpp`. Since the boot logo animates FROM center TO the MiniLogo's corner position, showing both simultaneously would cause visual overlap. The BootLogoApp calls `m_miniLogo->show()` just before transitioning, ensuring seamless handoff — the logo appears to shrink into its permanent corner position.

### [2026-02-16] Height-to-Width Conversion for VectorRenderer
`VectorRenderer::draw()` takes `width_percent` (percent of screen width), but the spec defines logo size in terms of screen height percentage. Conversion: `width_percent = height_percent * (screen_height / screen_width) * (logo_width / logo_height)`. This matches the formula used by `MiniLogo` and ensures the end state is pixel-identical to the permanent mini logo overlay.

### [2026-02-16] Main Application Setup (Updated 2026-02-17)
`main.cpp` boot sequence:
1. Display HAL + Touch HAL init.
2. RelativeDisplay + AnimationTicker + GestureEngine.
3. Create all UI components (PowerManager, StockTicker, MiniLogo, BootLogo, SystemMenu).
4. Register with UIRenderManager, `setActiveApp(g_bootLogo)` — boot logo starts on first `loop()`.
5. Launch background WiFi FreeRTOS task on Core 0 — signals `setBootComplete()` or `setErrorMessage()`.
6. On resume from suspend, the StockTicker is already the active app — BootLogo does NOT run again.

### [2026-02-16] Atomic Compose-and-Blit — Eliminates Animation Flashing
**Problem:** Dirty-rect erase via `gfx->fillRect()` followed by `VectorRenderer::draw()` still flashed — the display briefly showed the erased background between the two operations.
**Root cause:** Any two-step erase-then-draw sequence will flash because the display scanout can occur between the operations. The fix must ensure the display only ever sees a fully composed frame.
**Fix:** Adopted the same atomic blit pattern used by `TimeSeriesGraph::drawLiveIndicator()`:
1. Compute union bounding box of previous and current logo positions.
2. Create an off-screen `Arduino_Canvas` (PSRAM) sized to the union region.
3. Fill the canvas with the background color (erasing the old logo area).
4. Rasterize the logo triangles directly into the canvas using VectorRenderer's coordinate math, offset by the canvas origin.
5. Single atomic `hal_display_fast_blit()` writes the entire composed result to the display.
The first frame still uses a full-screen `drawSolidBackground()` + `VectorRenderer::draw()` since there is no prior content to flash against.

### [2026-02-17] HOLDING State and External Boot-Complete Signaling
**Problem:** Original state machine auto-transitioned ANIMATE → DONE, meaning the boot logo always ran for a fixed 3.5s regardless of WiFi timing. Spec §3.1 requires holding the animation until background init completes.
**Fix:** Added `HOLDING` state between ANIMATE and DONE. After animation finishes, the logo holds its final frame indefinitely until `setBootComplete()` is called from the background WiFi task. If WiFi connects before animation ends, HOLDING transitions to DONE immediately on the next `update()` cycle.

### [2026-02-17] Error Display (§3.3)
`setErrorMessage(const char* message)` sets a volatile pointer + flag. The `update()` loop checks `m_hasError` every frame (highest priority, overrides all states). On transition to ERROR state, `render()` calls `renderErrorScreen()` once — clears to theme background, centers the message using theme font/color (same pattern as `stock_ticker_app.cpp` status screens). The boot halts indefinitely in ERROR state.

### [2026-02-17] Background WiFi FreeRTOS Task
WiFi initialization moved from blocking `setup()` to a one-shot FreeRTOS task (`wifiTaskFunction`) pinned to Core 0 (WiFi stack core). Uses `vTaskDelay(pdMS_TO_TICKS(250))` for polling (not `delay()`/`yield()`). On success calls `setBootComplete()`, on all-fail calls `setErrorMessage("No WiFi Network Found")`. Self-deletes via `vTaskDelete(nullptr)`. Stack: 8KB (no TLS needed — just WiFi association).

### [2026-02-17] Cross-Core Thread Safety
Three volatile flags are written once by the WiFi task (Core 0) and read by `update()`/`render()` (Core 1):
- `volatile bool m_bootComplete` — write-once success signal
- `volatile bool m_hasError` — write-once error signal
- `const char* volatile m_errorMessage` — string literal pointer (flash memory)
No mutex needed: all are single-writer/single-reader, write-once, pointer-sized (atomic on ESP32-S3).
