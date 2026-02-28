# Implementation Notes: Firmware Boot Sequence

## Builder Decisions

[DECISION] Deep sleep wakeup gate placed before all initialization. On T-Display S3 Plus, which uses deep sleep for "shutdown," `hal_power_check_wakeup()` verifies intentional startup via sustained button hold. Returns immediately on cold boot or boards with hardware PEK. Re-enters deep sleep if hold is too short (never returns to caller).

[DECISION] 500ms post-serial delay is required. ESP32-S3 USB CDC needs time for host enumeration. Without this delay, early serial output is lost. The `yield()` call services the watchdog timer.

[DECISION] PowerManager init failure is non-fatal. Power monitoring is a "nice to have" — the device is fully functional without it. All other component failures are fatal because they affect core rendering or interaction.

[DECISION] SystemMenuComponent receives extensive theme configuration in Stage 4. This is intentional: the menu is a widget-based component that needs heading fonts, heading colors, underline style, list fonts, widget colors (normal, highlight, connecting background, error, scroll indicator), version font, version color, SSID font, and SSID color. All values come from the active theme.

[DECISION] WiFi task pinned to Core 0 (WiFi stack core). Core 1 runs the UI loop. The 8KB stack is sufficient because the task does no TLS — just WiFi connect/poll. The task self-deletes after completion to free the stack.

[DECISION] Cross-core signaling uses volatile booleans, not mutexes. The single-writer (Core 0) / single-reader (Core 1) pattern with volatile is sufficient. The error message pointer is written BEFORE the error flag to guarantee safe read ordering on Core 1.

[DECISION] Home button on CST816 touch controller is mapped to synthetic EDGE_DRAG DOWN gesture at bottom-center of screen. This allows the UIRenderManager input routing to handle it uniformly without special-casing physical buttons.

## Key Files

- `src/main.cpp` — Entry point: setup() and loop()
- `src/wifi_config_generated.h` — Build-time generated WiFi credentials (from config.json via inject_config.py)
- `src/animation_ticker.h` — 30 FPS frame timing
- `src/relative_display.h` — Coordinate transformation layer
- `src/input/touch_gesture_engine.h` — Gesture state machine
- `src/ui/ui_render_manager.h` — Singleton component registry
