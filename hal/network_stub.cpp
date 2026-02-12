/**
 * @file network_stub.cpp
 * @brief Stub implementation of Network HAL for testing
 *
 * Provides minimal functionality for native unit testing.
 */

#include "network.h"

// Stub state
static hal_network_status_t g_stub_status = HAL_NETWORK_STATUS_DISCONNECTED;
static bool g_stub_ping_result = false;

bool hal_network_init(const char* ssid, const char* password) {
    (void)ssid;
    (void)password;

    // Stub always succeeds and immediately connects
    g_stub_status = HAL_NETWORK_STATUS_CONNECTED;
    return true;
}

hal_network_status_t hal_network_get_status(void) {
    return g_stub_status;
}

bool hal_network_ping(const char* host) {
    (void)host;
    return g_stub_ping_result;
}

const char* hal_network_get_ssid(void) {
    return "Demo WiFi";
}

bool hal_network_http_get(const char* url, char* response_buffer, size_t buffer_size) {
    (void)url;
    (void)buffer_size;

    // Stub returns empty response
    if (response_buffer != nullptr && buffer_size > 0) {
        response_buffer[0] = '\0';
    }
    return false;
}

// Test helper functions (not part of HAL API)
#ifdef UNIT_TEST
void hal_network_stub_set_status(hal_network_status_t status) {
    g_stub_status = status;
}

void hal_network_stub_set_ping_result(bool result) {
    g_stub_ping_result = result;
}
#endif
