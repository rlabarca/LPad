/**
 * @file demo_v05_entry.h
 * @brief Release 0.5 Demo Entry Point
 *
 * Provides setup() and loop() implementation for v0.5 demo.
 * See features/demo_release_0.5.md for specification.
 */

#ifndef DEMO_V05_ENTRY_H
#define DEMO_V05_ENTRY_H

/**
 * @brief Initialize the v0.5 demo application
 *
 * Sets up:
 * - Display HAL
 * - RelativeDisplay
 * - AnimationTicker (30fps)
 * - V05DemoApp (Logo + 6 Graph Modes)
 */
void demo_setup();

/**
 * @brief Main loop for v0.5 demo
 *
 * Updates and renders V05DemoApp, restarts when cycle completes.
 */
void demo_loop();

#endif // DEMO_V05_ENTRY_H
