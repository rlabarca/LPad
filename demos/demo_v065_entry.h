/**
 * @file demo_v065_entry.h
 * @brief Release 0.65 Demo Entry Point
 *
 * Release 0.65 adds touch interaction and debug overlay on top of v0.60.
 */

#ifndef DEMO_V065_ENTRY_H
#define DEMO_V065_ENTRY_H

/**
 * @brief Setup function for Release 0.65 demo
 *
 * Initializes hardware (display, touch), and V065DemoApp.
 * Called once by main.cpp setup().
 */
void demo_setup();

/**
 * @brief Loop function for Release 0.65 demo
 *
 * Updates and renders the demo at 30fps with touch interaction.
 * Called repeatedly by main.cpp loop().
 */
void demo_loop();

#endif // DEMO_V065_ENTRY_H
