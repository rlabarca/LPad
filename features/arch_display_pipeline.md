# Architecture: Display Pipeline

> Label: "Architecture: Display Pipeline"
> Category: "Architecture"

## Purpose

Defines the display rendering pipeline constraints for LPad firmware. All visual output uses RGB565 packed pixel format, flows through resolution-independent relative coordinates (0-100%), and may use PSRAM-backed canvas layers for compositing. The pipeline supports tear-free updates via atomic blit operations and maintains a shadow framebuffer for diagnostic screenshot capture.

## Display Pipeline Invariants

### Pixel Format

- All color values throughout the codebase MUST use RGB565 (16-bit packed: 5 red, 6 green, 5 blue).
- Color constants are defined in theme files as `uint16_t` RGB565 values. No runtime color space conversion exists.

### Resolution Independence

- Application-level drawing code MUST use relative coordinates (0.0-100.0 float, representing percentage of screen dimensions) via `RelativeDisplay`.
- Only HAL implementations and `RelativeDisplay` internals may use absolute pixel coordinates.
- This ensures the same application code runs on both portrait (368x448) and landscape (448x368) display orientations without modification.

### Canvas and Compositing

- Large off-screen buffers (canvases) MUST be allocated in PSRAM via `hal_display_canvas_create()`.
- Multi-layer rendering (e.g., TimeSeriesGraph's background/data/main layers) composites canvases using `hal_display_fast_blit()` or `hal_display_fast_blit_transparent()`.
- Blit operations MUST use DMA transfer. Pixel-by-pixel loops are prohibited for canvas-to-display transfers.

### Tear Prevention and Dirty-Rect Animation

- Any rendering that updates a region where the previous frame's content must be fully replaced MUST use atomic blit (single DMA transfer covering both old and new regions).
- All animated content (moving sprites, pulsing indicators, cycling text) MUST use dirty-rect blitting: track the previous frame's bounding box, compute the union of old and new bounding boxes, composite the updated region into a temporary canvas, and DMA blit the result as a single atomic operation.
- Direct draw-erase-redraw sequences on the live display are PROHIBITED for animated content -- they cause visible flicker on the AMOLED panels.
- Features that render animated content MUST declare `> Prerequisite: features/arch_display_pipeline.md` and follow this dirty-rect pattern.

### Shadow Framebuffer

- Every pixel written to the display hardware MUST also be written to the PSRAM shadow framebuffer.
- The shadow framebuffer enables the serial screenshot feature (`hal_display_dump_screen()`).
- `hal_display_read_pixel()` reads from the shadow framebuffer, not from display hardware.

## Scenarios

No automated or manual scenarios. This is a policy anchor node -- its "scenarios" are
process invariants enforced by instruction files and tooling.
