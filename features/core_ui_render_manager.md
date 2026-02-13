# Core Feature: UI Render Manager

> Label: "UI Render Manager"
> Category: "System Architecture"
> Prerequisite: features/touch_gesture_engine.md
> Prerequisite: features/arch_ui_compositing.md

## 1. Overview
The `UIRenderManager` is the central singleton responsible for managing the application lifecycle, rendering pipeline, and touch event distribution. It decouples individual UI components from the main loop, allowing for a composable system where Apps and System Components can be registered, prioritized (Z-Order), and managed dynamically.

## 2. Terminology
*   **UIComponent**: Abstract base class for all renderable/interactive elements.
*   **AppComponent**: A component that represents a full "Application" (e.g., Stock Ticker, Settings). Only one can be Active/Running at a time.
*   **SystemComponent**: A component that persists globally (e.g., System Menu, Status Bar, Mini Logo). Multiple can run simultaneously.
*   **Z-Order**: Integer priority. Higher numbers draw *later* (on top).
*   **Activation Event**: A specific touch gesture (e.g., `EDGE_DRAG: TOP`) that triggers a SystemComponent to wake up.

## 3. Architecture & Data Structures

### 3.1 Component Registry
The Manager maintains a registry of components sorted by Z-Order.
*   **Constraint:** No two components can register with the same Z-Order. Registration MUST fail/error if a duplicate Z-Order is attempted.
*   **Storage:** A sorted list or vector of `UIComponent*`.

### 3.2 The Render Loop (Painter's Algorithm)
On every system tick (frame), the main loop calls `routeInput()`, `update()`, and `render()`.

1.  **Input Routing:** `routeInput()` dispatches touch events to the active component stack.
2.  **State Update:** `update()` advances animations for all active components.
3.  **Rendering:** `render()` iterates through the registered components in ascending Z-Order (Lowest -> Highest).
    *   **Occlusion Check:** The Manager tracks if the screen is "Fully Occluded" by a higher-priority component.
    *   *Note:* This requires components to report their `isOpaque()` and `isFullscreen()` state.
    *   If a higher component is opaque and full-screen, lower components are skipped (Performance Optimization).
    *   If not occluded and component is `Visible`, call `component->render()`.

### 3.3 Event Routing
The Manager receives gesture events from `main.cpp` via `routeInput()`. The `TouchGestureEngine` is owned and driven by `main.cpp`, not by the Manager.
1.  **Global Activation Check:** Incoming events are first checked against registered **Activation Events** for SystemComponents.
    *   If Match: The target SystemComponent is `UnPaused` (if paused) and the event is consumed.
2.  **Active Component Dispatch:** If no global activation matches, the event is passed to the currently **Active** components (Unpaused SystemComponents and the Running App), starting from Highest Z-Order to Lowest.
    *   If a component consumes the event (returns `true`), propagation stops.

## 4. Lifecycle Methods

### 4.1 UIComponent Interface
*   **Initialization:** Components are initialized externally via per-component `begin()` methods (e.g., `begin(RelativeDisplay*)`, `begin(Arduino_GFX*, width, height)`) **before** registration with the Manager. The Manager does not call any init method during `registerComponent()`.
*   `onRun()`: Called by the Manager on an AppComponent when `setActiveApp()` is invoked. **Not called on SystemComponents** — passive SystemComponents rely on default `m_visible = true`.
*   `onPause()`: Called when the component is backgrounded/suspended.
*   `onUnpause()`: Called when the component is resumed (e.g., SystemComponent activation, or App resumed after SystemComponent yields).
*   `render()`: Called every frame if visible, not paused, and not occluded.
*   `update(float dt)`: Called every frame with delta time for animations (e.g., live indicator pulse, menu close animation).
*   `handleInput(const touch_gesture_event_t& event)`: Called when input is routed to this component. Returns `true` to consume, `false` to pass through.

### 4.2 AppComponent Specifics
*   `onClose()`: Called when the App is shut down entirely (to free memory).

### 4.3 SystemComponent Specifics
*   `show()`: Helper to set visibility and unpause.
*   `hide()`: Helper to hide and pause.
*   `systemPause()`: Explicitly yields control back to the Manager.

## 5. Scenarios

### Scenario: Registration and Z-Order Enforcement
    Given the RenderManager is initialized
    When I register a component "Background" with Z-Order 0
    And I register a component "Ticker" with Z-Order 1
    Then the registration succeeds
    When I attempt to register "Status" with Z-Order 1
    Then the registration fails with a "Duplicate Z-Order" error

### Scenario: App Switching (Pause/Resume)
    Given "StockTicker" is the running App (Z=1)
    And "SystemMenu" is a registered SystemComponent (Z=20)
    And "SystemMenu" is configured with ActivationEvent "EDGE_DRAG: TOP"
    When an "EDGE_DRAG: TOP" gesture is detected
    Then "StockTicker" receives `onPause()`
    And "SystemMenu" receives `onUnpause()` and `show()`
    And "SystemMenu" becomes the exclusive recipient of touch events

### Scenario: System Menu Closing
    Given "SystemMenu" is active and "StockTicker" is paused
    When "SystemMenu" calls `systemPause()` (e.g., after a Close gesture)
    Then "SystemMenu" receives `onPause()`
    And "StockTicker" receives `onUnpause()`
    And "StockTicker" resumes receiving touch events

### Scenario: Rendering Order and Occlusion
    Given "StockTicker" (Z=1) is running
    And "MiniLogo" (Z=10) is running and Visible
    And "SystemMenu" (Z=20) is running and Visible
    When the render loop executes
    Then "StockTicker" `render()` is NOT called (Occluded by SystemMenu)
    And "MiniLogo" `render()` is NOT called (Occluded by SystemMenu)
    And "SystemMenu" `render()` IS called
    *Note: Assumes SystemMenu reports itself as Opaque + FullScreen*

### Scenario: Transparent Overlay Rendering
    Given "StockTicker" (Z=1) is running
    And "MiniLogo" (Z=10) is running
    And "SystemMenu" is Paused/Hidden
    When the render loop executes
    Then "StockTicker" `render()` IS called first
    And "MiniLogo" `render()` IS called second (drawing on top)
