/**
 * @file demo_v060_entry.h
 * @brief Release 0.60 Demo Entry Point
 */

#ifndef DEMO_V060_ENTRY_H
#define DEMO_V060_ENTRY_H

/**
 * @brief Setup function for Release 0.60 demo
 *
 * Initializes hardware, display, and V060DemoApp.
 * Called once by main.cpp setup().
 */
void demo_setup();

/**
 * @brief Loop function for Release 0.60 demo
 *
 * Updates and renders the demo at 30fps.
 * Called repeatedly by main.cpp loop().
 */
void demo_loop();

#endif // DEMO_V060_ENTRY_H
