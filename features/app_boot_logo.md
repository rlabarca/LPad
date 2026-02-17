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

### [NEW] App Chaining Pattern
This feature introduces a simple "App Chaining" pattern. The `BootLogo` app will be initialized with a pointer to the next app to run (the `StockTickerApp`). In its `update()` method, once the animation is complete, it will execute:
`m_render_manager->setActiveApp(m_next_app);`
This delegates the responsibility of app sequencing to the apps themselves, allowing for simple, linear flows without requiring a complex state machine in `main.cpp`. The `UIRenderManager` handles the `onClose()` and `onRun()` calls automatically.

### Main Application Setup
`main.cpp` must be modified to support this flow:
1.  Instantiate `UIRenderManager`.
2.  Instantiate `StockTickerApp`.
3.  Instantiate `BootLogoApp`, passing it pointers to the manager and the stock ticker app.
4.  Call `manager->setActiveApp()` with the `BootLogoApp` instance.
5.  Register other `SystemComponent`s as usual.
