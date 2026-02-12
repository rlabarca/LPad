# Release 0.70: UI Render Manager

> Label: "Release v0.70: UI Render Manager"
> Category: "RELEASES"
> Prerequisite: features/core_ui_render_manager.md
> Prerequisite: features/sys_system_menu.md
> Prerequisite: features/sys_mini_logo.md
> Prerequisite: features/app_stock_ticker.md

## 1. Objective
Introduce a managed UI architecture that allows multiple independent components (Apps and System Tools) to coexist, managing rendering order (Z-Order) and input routing. Move away from monolithic "Demo Apps" to a "System + App" model.

## 2. Integration Requirements

### 2.1 The "LPad" Application
*   The `main.cpp` loop is replaced by the `UIRenderManager::update()` loop.
*   **Startup Sequence:**
    1.  HAL Init (Display, Touch, Network).
    2.  `UIRenderManager` Init.
    3.  Register `MiniLogo` (System, Z=10).
    4.  Register `SystemMenu` (System, Z=20).
    5.  Register `StockTicker` (App, Z=1).
    6.  `UIRenderManager->runApp("StockTicker")`.

### 2.2 Migration
*   Existing logic in `demos/v060_demo_app.cpp` moves to `src/apps/stock_ticker.cpp`.
*   Existing logic in `demos/v067_demo_app.cpp` (Menu) moves to `src/system/system_menu.cpp`.
*   `MiniLogo` becomes `src/system/mini_logo.cpp`.

## 3. Verification Criteria

### 3.1 HIL Test: Compositing
*   **Step 1:** Boot device. Verify Stock Graph appears (Z=1) and Mini Logo appears (Z=10) on top.
*   **Step 2:** verify Logo background is transparent (no black box obscuring graph).

### 3.2 HIL Test: System Menu Interaction
*   **Step 1:** Perform `EDGE_DRAG: TOP`.
*   **Step 2:** Verify System Menu slides down/appears (Z=20).
*   **Step 3:** Verify Graph stops updating (Paused/Occluded).
*   **Step 4:** Perform `EDGE_DRAG: BOTTOM`.
*   **Step 5:** Verify Menu disappears.
*   **Step 6:** Verify Graph resumes updating.
