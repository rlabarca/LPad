# Feature: App Component - Boot Logo

> Label: "App Component: Boot Logo"
> Category: "Applications"
> Prerequisite: features/core_ui_render_manager.md
> Prerequisite: features/ui_vector_assets.md
> Prerequisite: features/app_stock_ticker.md

## 1. Overview
This feature defines a dedicated `AppComponent` for the boot logo animation. This App runs exactly once on initial device power-on, plays a full-screen logo animation, and then transitions control to the main application (e.g., Stock Ticker).

## 2. Configuration
*   **Type:** `AppComponent`
*   **Z-Order:** 5 (Above default App Z-Order, below System Components)
*   **Name:** "BootLogo"

## 3. Behavior

### 3.1 Lifecycle & Animation
The component's lifecycle is based on the animation sequence defined in the legacy `ui_logo_screen.md` feature.

*   **`onRun()`:**
    *   Called by the `UIRenderManager` when the `BootLogo` is set as the initial active app.
    *   Begins the animation state machine.
*   **State Machine:**
    1.  **WAIT:** Logo remains static in the center for 2 seconds.
    2.  **ANIMATE:** Logo smoothly interpolates position and scale over 1.5 seconds.
    3.  **DONE:** Animation is complete.
*   **`update(float dt)`:**
    *   Advances the state machine's internal timers.
    *   When the state transitions to `DONE`, it triggers the transition to the next application.
*   **`render()`:**
    *   Draws the logo using `VectorRenderer` based on the current animation parameters (position, scale, anchor).
    *   The background is filled with the theme's `THEME_BACKGROUND` color.
*   **`onClose()`:** Called by the `UIRenderManager` when the app is replaced. Frees any resources.

### 3.2 Visual Parameters
*   **Start State:**
    *   Position: Centered on screen.
    *   Size: 75% of screen height.
    *   Anchor: Center (0.5, 0.5).
*   **End State:**
    *   Position: Top-right corner with a 10px buffer from the top and right edges.
    *   Size: 10% of screen height.
    *   Anchor: Top-right (1.0, 0.0).
*   **Easing:** `EaseInOutCubic`.

### 3.3 App Transition
*   The `BootLogo` app is responsible for handing control to the next application.
*   When its animation state becomes `DONE`, it must call `UIRenderManager::getInstance().setActiveApp()` to switch to the main application.

## 4. Scenarios

### Scenario: Initial Boot Sequence
    Given the device has just powered on
    And the RenderManager is initialized
    And the "BootLogo" app is set as the initial active app
    When the main loop runs
    Then the "BootLogo" app displays its full-screen animation
    And upon completion, the "BootLogo" app calls `setActiveApp()` to switch to the "StockTicker" app
    And the "BootLogo" app is closed and removed from the active stack
    And the "StockTicker" app begins running

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

### [2026-02-16] Main Application Setup
`main.cpp` boot sequence:
1. Instantiate `StockTickerApp` and `MiniLogoComponent`.
2. Instantiate `BootLogoApp`, passing `RelativeDisplay*`, `StockTickerApp*`, and `MiniLogoComponent*`.
3. Register all components: PowerManager(Z=0), StockTicker(Z=1), BootLogo(Z=5), MiniLogo(Z=10), SystemMenu(Z=20).
4. Hide MiniLogo, then call `mgr.setActiveApp(g_bootLogo)` to start the boot animation.
5. On resume from suspend, the StockTicker is already the active app — BootLogo does NOT run again.
