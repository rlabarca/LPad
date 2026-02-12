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
 * - DEMO_V058: Release 0.58 Demo (WiFi + Logo + 6 Graph Modes + Live Data)
 * - DEMO_V060: Release 0.60 Demo (Logo + WiFi + Stock Tracker ^TNX)
 * - DEMO_V065: Release 0.65 Demo (v0.60 + Touch Interaction)
 * - DEMO_V067: Release 0.67 Demo (v0.60 + System Menu)
 *
 * The actual setup() and loop() implementations live in:
 * - demos/demo_v05_entry.cpp  (for v0.5)
 * - demos/demo_v055_entry.cpp (for v0.55)
 * - demos/demo_v058_entry.cpp (for v0.58)
 * - demos/demo_v060_entry.cpp (for v0.60)
 * - demos/demo_v065_entry.cpp (for v0.65)
 * - demos/demo_v067_entry.cpp (for v0.67)
 */

// Conditional compilation: select the demo entry point
#if defined(DEMO_V05)
    #include "../demos/demo_v05_entry.h"
    #define DEMO_NAME "v0.5"
#elif defined(DEMO_V055)
    #include "../demos/demo_v055_entry.h"
    #define DEMO_NAME "v0.55"
#elif defined(DEMO_V058)
    #include "../demos/demo_v058_entry.h"
    #define DEMO_NAME "v0.58"
#elif defined(DEMO_V060)
    #include "../demos/demo_v060_entry.h"
    #define DEMO_NAME "v0.60"
#elif defined(DEMO_V065)
    #include "../demos/demo_v065_entry.h"
    #define DEMO_NAME "v0.65"
#elif defined(DEMO_V067)
    #include "../demos/demo_v067_entry.h"
    #define DEMO_NAME "v0.67"
#else
    #error "No demo entry point defined. Set -DDEMO_V05, -DDEMO_V055, -DDEMO_V058, -DDEMO_V060, -DDEMO_V065, or -DDEMO_V067 in platformio.ini"
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
