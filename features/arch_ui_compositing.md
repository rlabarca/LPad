# Architectural Policy: UI Compositing

> Label: "UI Compositing Policy"
> Category: "ARCHITECTURES"
> Prerequisite: None

## 1. Relative Coordinates
The Application Layer operates exclusively in a **0.0 to 1.0** float coordinate space.
*   `x=0.0` is Left, `x=1.0` is Right.
*   `y=0.0` is Bottom, `y=1.0` is Top.
*   The `RelativeDisplay` module is the *only* component authorized to translate these floats into integer pixel coordinates.

## 2. Layered Rendering (The "Canvas" Strategy)
To prevent flickering and support complex composition, we use an off-screen canvas strategy.
*   **Background Layer:** Static elements (grid, axes) are drawn once to a persistent canvas.
*   **Data Layer:** Dynamic elements (graphs) are drawn to a separate canvas.
*   **Composition:** The HAL is responsible for blitting these canvases to the physical display using `hal_display_fast_blit`.

## 3. Partial Updates
Full screen clears (`hal_display_clear`) are **prohibited** in the `loop()`. Updates must use dirty-rect logic or optimized blitting of small regions (e.g., the pulsing indicator).

## 4. Overlay Pattern
For overlaying static elements (e.g., title text) on frequently-updating dynamic content:
1.  Pre-render overlay content to a PSRAM buffer using `Arduino_Canvas`.
2.  Fill canvas with chroma key `0x0001` for transparency.
3.  Cache the rendered buffer (invalidate on content change).
4.  Use `hal_display_fast_blit_transparent` to composite overlay.

## 5. UI Render Manager (Z-Order)
*   **Painter's Algorithm:** Components are rendered in ascending **Z-Order** (0 to N). Higher Z-order components draw on top of lower ones.
*   **No Master Framebuffer:** Components must draw to their own off-screen surfaces or directly to the display.
*   **Occlusion Optimization:** If a high Z-order component reports `isOpaque = true` and `isFullscreen = true`, lower components are skipped to conserve resources.
