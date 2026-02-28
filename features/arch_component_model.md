# Architecture: Component Model

> Label: "Architecture: Component Model"
> Category: "Architecture"

## Purpose

Defines the Z-ordered component architecture that structures all visual and interactive elements in the LPad UI. The `UIRenderManager` singleton maintains a flat array of components sorted by Z-order, renders them using the Painter's Algorithm (back-to-front), and routes input events in reverse Z-order (front-to-back). Components are classified as `AppComponent` (full-screen applications) or `SystemComponent` (overlays with activation gestures).

## Component Model Invariants

### Component Hierarchy

- All renderable UI elements MUST extend either `AppComponent` or `SystemComponent`, both of which extend the abstract `UIComponent` base class.
- `AppComponent`: Full-screen applications. Only one may be active at a time (set via `UIRenderManager::setActiveApp()`).
- `SystemComponent`: Overlay components with activation gestures (e.g., edge drag). May be shown/hidden independently.

### Z-Order Rules

- The UIRenderManager supports a maximum of 16 registered components.
- Components are rendered in ascending Z-order (Painter's Algorithm: lowest Z painted first, highest Z on top).
- Input events are routed in descending Z-order (highest Z gets first chance to consume the event).
- The current Z-order assignment: Z=0 PowerManager, Z=1 StockTickerApp, Z=5 BootLogoApp, Z=10 MiniLogoComponent, Z=20 SystemMenuComponent.

### Lifecycle Contracts

- `onRun()`: Called when the component becomes active. Must initialize rendering state.
- `onPause()`: Called before the component is deactivated or the system suspends. Must stop background work.
- `onUnpause()`: Called when resuming. Must trigger a full redraw (AMOLED GRAM is undefined after sleep).
- `onClose()`: Called when the component is permanently removed.
- `render()`: Called every frame for visible components. Must draw the component's current state.
- `update(float dt)`: Called every frame. `dt` is seconds since last frame.
- `handleInput(event)`: Called with touch gesture events. Return `true` to consume the event.

### Occlusion Optimization

- The render manager calculates an "occlusion floor": the highest-Z component that is both opaque and fullscreen.
- Components below the occlusion floor are skipped during rendering (their pixels would be completely overwritten).
- Components MUST accurately report `isOpaque()` and `isFullscreen()` for this optimization to work correctly.

## Scenarios

No automated or manual scenarios. This is a policy anchor node -- its "scenarios" are
process invariants enforced by instruction files and tooling.
