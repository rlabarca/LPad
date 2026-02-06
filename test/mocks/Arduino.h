/**
 * @file Arduino.h
 * @brief Minimal Arduino API stubs for native unit tests
 */

#pragma once

#include <stdint.h>
#include <thread>
#include <chrono>

#ifdef __cplusplus
extern "C" {
#endif

// Mock delay function for native testing
inline void delay(unsigned long ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Mock delayMicroseconds function for native testing
inline void delayMicroseconds(unsigned int us) {
    std::this_thread::sleep_for(std::chrono::microseconds(us));
}

#ifdef __cplusplus
}
#endif
