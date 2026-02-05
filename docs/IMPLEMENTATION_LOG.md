# Implementation Log & Lessons Learned

This document captures the "tribal knowledge" of the project: technical hurdles, why specific decisions were made, and what approaches were discarded.

## [2026-02-04] The "Flicker-Free" Graph Rendering Struggle

### Problem
When rendering the time-series graph, updating the "live indicator" (the pulsing dot) caused the entire graph line or background to flicker, or left "ghost" artifacts behind.

### Discarded Attempts
1.  **Full Redraw:** Clearing the screen and redrawing the entire graph every frame.
    *   *Result:* Unacceptable flicker at 30fps. The ESP32-S3 cannot push full 320x170 frames over SPI fast enough to beat the refresh rate.
2.  **XOR Drawing:** Attempting to "undraw" the previous dot using XOR logic.
    *   *Result:* Colors looked wrong on the gradient background.
3.  **Single Buffer w/ Invalidation:** Drawing directly to the screen and trying to "erase" just the dot by drawing a black rectangle over it.
    *   *Result:* Erased the graph line and background gradient behind the dot, creating "black holes."

### Final Solution: Layered Canvas + Partial Restore
We adopted a "Layered Rendering" strategy similar to game engines:
1.  **Background Canvas:** We allocate a PSRAM buffer for the static background (gradients + grid). This is drawn *once*.
2.  **Data Canvas:** We allocate a PSRAM buffer for the data line. This is drawn *once*.
3.  **Composition:** On `update()`, we:
    *   Calculate the bounding box of the *previous* dot frame.
    *   Blit the "clean" pixels from the Background and Data canvases *only* into that bounding box (effectively "restoring" the background).
    *   Draw the *new* dot at the new size/position.
4.  **Hardware:** This relies heavily on `hal_display_fast_blit` which uses DMA. Without DMA, this composition would be too slow.

### Key Lesson
**PSRAM is cheap; Bandwidth is expensive.** It is better to burn 150KB of PSRAM to store cached layers than to try to re-compute pixels or push full frames over the SPI bus.