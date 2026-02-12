/**
 * @file demo_main.cpp
 * @brief Legacy Demo Dispatcher (v0.5 through v0.67)
 *
 * Contains the #ifdef DEMO_VXX dispatcher pattern, moved from main.cpp.
 * Used only by demo_v05_* through demo_v067_* build targets.
 * The new v0.70+ architecture uses src/main.cpp directly.
 */

// Conditional compilation: select the demo entry point
#if defined(DEMO_V05)
    #include "demo_v05_entry.h"
    #define DEMO_NAME "v0.5"
#elif defined(DEMO_V055)
    #include "demo_v055_entry.h"
    #define DEMO_NAME "v0.55"
#elif defined(DEMO_V058)
    #include "demo_v058_entry.h"
    #define DEMO_NAME "v0.58"
#elif defined(DEMO_V060)
    #include "demo_v060_entry.h"
    #define DEMO_NAME "v0.60"
#elif defined(DEMO_V065)
    #include "demo_v065_entry.h"
    #define DEMO_NAME "v0.65"
#elif defined(DEMO_V067)
    #include "demo_v067_entry.h"
    #define DEMO_NAME "v0.67"
#else
    #error "demo_main.cpp requires a DEMO_VXX flag (-DDEMO_V05 through -DDEMO_V067)"
#endif

void setup() {
    demo_setup();
}

void loop() {
    demo_loop();
}
