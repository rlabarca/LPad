/**
 * @file demo_v055_entry.h
 * @brief Release 0.55 Demo Entry Point
 *
 * Provides setup() and loop() implementation for v0.55 demo.
 * See features/demo_release_0.55.md for specification.
 */

#ifndef DEMO_V055_ENTRY_H
#define DEMO_V055_ENTRY_H

/**
 * @brief Initialize the v0.55 demo application
 *
 * Sets up:
 * - Display HAL
 * - RelativeDisplay
 * - AnimationTicker (30fps)
 * - V055DemoApp (WiFi + Logo + 6 Graph Modes)
 */
void demo_setup();

/**
 * @brief Main loop for v0.55 demo
 *
 * Updates and renders V055DemoApp (connectivity + visual demo cycle).
 */
void demo_loop();

#endif // DEMO_V055_ENTRY_H
