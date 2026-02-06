/**
 * @file network_esp32.cpp
 * @brief ESP32 implementation of Network HAL
 *
 * Implements Wi-Fi connectivity using Arduino WiFi library.
 */

#include "network.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>

// Internal state
static hal_network_status_t g_status = HAL_NETWORK_STATUS_DISCONNECTED;

bool hal_network_init(const char* ssid, const char* password) {
    if (ssid == nullptr || password == nullptr) {
        g_status = HAL_NETWORK_STATUS_ERROR;
        return false;
    }

    // Set WiFi mode to station (client)
    WiFi.mode(WIFI_STA);

    // Start connection attempt
    WiFi.begin(ssid, password);
    g_status = HAL_NETWORK_STATUS_CONNECTING;

    return true;
}

hal_network_status_t hal_network_get_status(void) {
    // Update status based on WiFi state
    if (g_status == HAL_NETWORK_STATUS_CONNECTING) {
        wl_status_t wifi_status = WiFi.status();

        if (wifi_status == WL_CONNECTED) {
            g_status = HAL_NETWORK_STATUS_CONNECTED;
        } else if (wifi_status == WL_CONNECT_FAILED ||
                   wifi_status == WL_CONNECTION_LOST ||
                   wifi_status == WL_DISCONNECTED) {
            // Check if we've been trying for too long
            // For now, we'll let it keep trying
            // In a production system, you might want a timeout
        }
    } else if (g_status == HAL_NETWORK_STATUS_CONNECTED) {
        // Check if we've lost connection
        if (WiFi.status() != WL_CONNECTED) {
            g_status = HAL_NETWORK_STATUS_DISCONNECTED;
        }
    }

    return g_status;
}

bool hal_network_ping(const char* host) {
    // Check if we're connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[hal_network_ping] WiFi not connected");
        return false;
    }

    // Use HTTP HEAD request as a simple connectivity test
    // If host is an IP address (like 8.8.8.8), use a known HTTP endpoint instead
    HTTPClient http;
    String url;

    // Check if host looks like an IP address (contains only digits and dots)
    bool isIP = true;
    for (size_t i = 0; i < strlen(host); i++) {
        if (!isdigit(host[i]) && host[i] != '.') {
            isIP = false;
            break;
        }
    }

    // If it's an IP, use google.com as fallback (8.8.8.8 is DNS, not HTTP)
    if (isIP) {
        url = "http://google.com";
        Serial.printf("[hal_network_ping] IP address detected, using %s instead\n", url.c_str());
    } else {
        url = String("http://") + host;
        Serial.printf("[hal_network_ping] Testing connectivity to %s\n", url.c_str());
    }

    http.begin(url);
    http.setTimeout(5000);  // 5 second timeout

    int httpCode = http.GET();
    http.end();

    Serial.printf("[hal_network_ping] HTTP response code: %d\n", httpCode);

    // Consider any response (even error codes) as connectivity success
    // We just want to know if we can reach the internet
    return (httpCode > 0);
}
