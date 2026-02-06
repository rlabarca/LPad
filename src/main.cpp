/**
 * @file main.cpp
 * @brief LPad Main Entry Point Dispatcher
 *
 * This file serves as a dispatcher that selects the appropriate demo
 * entry point based on build flags set in platformio.ini.
 *
 * Build flags:
 * - DEMO_V05: Release 0.5 Demo (Logo + 6 Graph Modes)
 * - DEMO_V055: Release 0.55 Demo (WiFi + Logo + 6 Graph Modes)
 *
 * The actual setup() and loop() implementations live in:
 * - demos/demo_v05_entry.cpp  (for v0.5)
 * - demos/demo_v055_entry.cpp (for v0.55)
 */

// Conditional compilation: select the demo entry point
#if defined(DEMO_V05)
    #include "../demos/demo_v05_entry.h"
    #define DEMO_NAME "v0.5"
#elif defined(DEMO_V055)
    #include "../demos/demo_v055_entry.h"
    #define DEMO_NAME "v0.55"
#else
    #error "No demo entry point defined. Set -DDEMO_V05 or -DDEMO_V055 in platformio.ini"
#endif

/**
 * @brief Arduino setup function - forwards to selected demo entry point
 */
void setup() {
    demo_setup();
}

/**
 * @brief Arduino loop function - forwards to selected demo entry point
 */
void loop() {
    demo_loop();
}
