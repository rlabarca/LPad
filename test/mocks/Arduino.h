/**
 * @file Arduino.h
 * @brief Minimal Arduino API stubs for native unit tests
 */

#pragma once

#include <stdint.h>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdarg>

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

// Minimal Serial stub for native testing
class HardwareSerial {
public:
    void begin(int baud) { (void)baud; }
    void println(const char* s = "") { (void)s; }
    void println(int v) { (void)v; }
    void print(const char* s) { (void)s; }
    void printf(const char* fmt, ...) { (void)fmt; }
    void flush() {}
};

inline HardwareSerial Serial;
