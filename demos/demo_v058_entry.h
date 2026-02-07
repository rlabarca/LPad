/**
 * @file demo_v058_entry.h
 * @brief Release 0.58 Demo Entry Point
 *
 * Provides setup() and loop() implementation for v0.58 demo.
 * See features/demo_release_0.58.md for specification.
 */

#ifndef DEMO_V058_ENTRY_H
#define DEMO_V058_ENTRY_H

/**
 * @brief Initialize the v0.58 demo application
 *
 * Sets up:
 * - Display HAL
 * - RelativeDisplay
 * - AnimationTicker (30fps)
 * - V058DemoApp (Dynamic Data Layer)
 */
void demo_setup();

/**
 * @brief Main loop for v0.58 demo
 *
 * Updates and renders V058DemoApp (connectivity + dynamic visual demo).
 */
void demo_loop();

#endif // DEMO_V058_ENTRY_H
