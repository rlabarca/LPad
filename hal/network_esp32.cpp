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
#include <esp_task_wdt.h>

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

bool hal_network_http_get(const char* url, char* response_buffer, size_t buffer_size) {
    if (url == nullptr || response_buffer == nullptr || buffer_size == 0) {
        Serial.println("[hal_network_http_get] ERROR: Invalid parameters");
        return false;
    }

    // Check if we're connected
    wl_status_t wifi_status = WiFi.status();
    Serial.printf("[hal_network_http_get] WiFi status: %d\n", wifi_status);

    if (wifi_status != WL_CONNECTED) {
        Serial.printf("[hal_network_http_get] ERROR: WiFi not connected (status=%d)\n", wifi_status);
        return false;
    }

    // Log network info
    Serial.printf("[hal_network_http_get] Local IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[hal_network_http_get] Signal strength: %d dBm\n", WiFi.RSSI());

    HTTPClient http;
    Serial.printf("[hal_network_http_get] Fetching: %s\n", url);
    Serial.printf("[hal_network_http_get] Buffer size: %d bytes\n", buffer_size);

    // Begin HTTP connection
    Serial.println("[hal_network_http_get] Calling http.begin()...");
    bool begin_result = http.begin(url);
    if (!begin_result) {
        Serial.println("[hal_network_http_get] ERROR: http.begin() failed");
        return false;
    }
    Serial.println("[hal_network_http_get] http.begin() succeeded");

    http.setTimeout(10000);  // 10 second timeout
    Serial.println("[hal_network_http_get] Timeout set to 10000ms");

    Serial.println("[hal_network_http_get] Sending GET request...");
    int httpCode = http.GET();
    Serial.printf("[hal_network_http_get] GET returned code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
        Serial.println("[hal_network_http_get] HTTP_CODE_OK received, getting payload...");

        // Get content length to check buffer size
        int contentLength = http.getSize();
        Serial.printf("[hal_network_http_get] Content-Length: %d bytes\n", contentLength);

        if (contentLength > 0 && contentLength >= (int)buffer_size) {
            Serial.printf("[hal_network_http_get] ERROR: Response too large: %d bytes (buffer: %d)\n",
                         contentLength, buffer_size);
            http.end();
            return false;
        }

        // Use streaming to read response in chunks and feed watchdog
        WiFiClient *stream = http.getStreamPtr();
        size_t bytes_read = 0;
        unsigned long last_wdt_feed = millis();

        Serial.println("[hal_network_http_get] Reading stream in chunks...");

        while (http.connected() && (contentLength > 0 || contentLength == -1)) {
            size_t available = stream->available();

            if (available) {
                // Read chunk (up to remaining buffer space)
                size_t chunk_size = min(available, buffer_size - bytes_read - 1);
                int c = stream->readBytes(response_buffer + bytes_read, chunk_size);

                if (c > 0) {
                    bytes_read += c;

                    if (contentLength > 0) {
                        contentLength -= c;
                    }

                    // Feed watchdog every 100ms to prevent timeout
                    if (millis() - last_wdt_feed > 100) {
                        esp_task_wdt_reset();
                        last_wdt_feed = millis();
                        Serial.printf("[hal_network_http_get] Read %d bytes (feeding watchdog)...\n", bytes_read);
                    }
                }
            } else if (contentLength <= 0) {
                break;  // All data received
            } else {
                delay(1);  // Wait for more data
            }

            // Check buffer overflow
            if (bytes_read >= buffer_size - 1) {
                Serial.printf("[hal_network_http_get] ERROR: Buffer overflow at %d bytes\n", bytes_read);
                http.end();
                return false;
            }
        }

        response_buffer[bytes_read] = '\0';  // Ensure null termination
        Serial.printf("[hal_network_http_get] SUCCESS: %d bytes received and copied\n", bytes_read);

        http.end();
        return true;
    } else {
        // Detailed error reporting
        Serial.printf("[hal_network_http_get] ERROR: HTTP error code: %d\n", httpCode);

        if (httpCode == HTTPC_ERROR_CONNECTION_REFUSED) {
            Serial.println("[hal_network_http_get] ERROR: Connection refused");
        } else if (httpCode == HTTPC_ERROR_SEND_HEADER_FAILED) {
            Serial.println("[hal_network_http_get] ERROR: Send header failed");
        } else if (httpCode == HTTPC_ERROR_SEND_PAYLOAD_FAILED) {
            Serial.println("[hal_network_http_get] ERROR: Send payload failed");
        } else if (httpCode == HTTPC_ERROR_NOT_CONNECTED) {
            Serial.println("[hal_network_http_get] ERROR: Not connected");
        } else if (httpCode == HTTPC_ERROR_CONNECTION_LOST) {
            Serial.println("[hal_network_http_get] ERROR: Connection lost");
        } else if (httpCode == HTTPC_ERROR_NO_STREAM) {
            Serial.println("[hal_network_http_get] ERROR: No stream");
        } else if (httpCode == HTTPC_ERROR_NO_HTTP_SERVER) {
            Serial.println("[hal_network_http_get] ERROR: No HTTP server");
        } else if (httpCode == HTTPC_ERROR_TOO_LESS_RAM) {
            Serial.println("[hal_network_http_get] ERROR: Too less RAM");
        } else if (httpCode == HTTPC_ERROR_ENCODING) {
            Serial.println("[hal_network_http_get] ERROR: Encoding error");
        } else if (httpCode == HTTPC_ERROR_STREAM_WRITE) {
            Serial.println("[hal_network_http_get] ERROR: Stream write failed");
        } else if (httpCode == HTTPC_ERROR_READ_TIMEOUT) {
            Serial.println("[hal_network_http_get] ERROR: Read timeout");
        } else if (httpCode > 0) {
            Serial.printf("[hal_network_http_get] HTTP status code: %d\n", httpCode);
        }

        http.end();
        return false;
    }
}
