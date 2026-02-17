/**
 * @file network_esp32.cpp
 * @brief ESP32 implementation of Network HAL
 *
 * Implements Wi-Fi connectivity using Arduino WiFi library.
 *
 * WiFi lifecycle notes:
 *   WiFi.disconnect(true) calls esp_wifi_stop() which shuts down the radio
 *   asynchronously. A subsequent WiFi.mode(WIFI_STA) restarts it, but
 *   WiFi.begin() can race the async WIFI_EVENT_STA_START event, causing
 *   "STA config failed". To avoid this, we only use disconnect(true) in
 *   hal_network_disconnect() (the intentional shutdown path). All connection
 *   paths use WiFi.disconnect(false) which disconnects from the AP but keeps
 *   the radio running, so WiFi.begin() can configure immediately.
 */

#include "network.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>

// Internal state
static hal_network_status_t g_status = HAL_NETWORK_STATUS_DISCONNECTED;
static char g_ssid_buffer[64] = "N/A";

// Stored credentials for reconnect after suspend/resume
static char g_stored_ssid[64] = "";
static char g_stored_password[64] = "";

// Connection timeout tracking
static unsigned long g_connect_start_ms = 0;
static constexpr unsigned long CONNECT_TIMEOUT_MS = 10000; // 10 seconds

// One-time WiFi subsystem init flag
static bool g_wifi_started = false;

/**
 * @brief Ensure the WiFi radio is started in STA mode and ready for connections.
 *
 * On first call: sets persistent(false) to stop NVS auto-connect on future
 * reboots, enables STA mode, and waits for the driver to report ready.
 * On subsequent calls: disconnects any active/pending connection without
 * stopping the radio, so WiFi.begin() can configure immediately.
 */
static void ensureWiFiReady(void) {
    if (!g_wifi_started) {
        // First call — full init. persistent(false) must be set BEFORE
        // WiFi.mode() to prevent credentials from being saved to NVS.
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);

        // Wait for the STA_START event. WiFi.begin() needs the driver fully
        // started; without this, esp_wifi_set_config() can fail.
        unsigned long start = millis();
        while (WiFi.status() == WL_NO_SHIELD && millis() - start < 2000) {
            delay(10);
        }

        g_wifi_started = true;
        Serial.println("[hal_network] WiFi subsystem started (STA mode, persistent=false)");
    }

    // Disconnect from any active or pending connection. wifioff=false keeps
    // the radio running so WiFi.begin() can configure without waiting for
    // another async STA_START event.
    WiFi.disconnect(false);
    delay(50);
}

bool hal_network_init(const char* ssid, const char* password) {
    if (ssid == nullptr || password == nullptr) {
        g_status = HAL_NETWORK_STATUS_ERROR;
        return false;
    }

    ensureWiFiReady();

    // Store credentials for reconnect after suspend/resume
    strncpy(g_stored_ssid, ssid, sizeof(g_stored_ssid) - 1);
    g_stored_ssid[sizeof(g_stored_ssid) - 1] = '\0';
    strncpy(g_stored_password, password, sizeof(g_stored_password) - 1);
    g_stored_password[sizeof(g_stored_password) - 1] = '\0';

    // Also store SSID in display buffer
    strncpy(g_ssid_buffer, ssid, sizeof(g_ssid_buffer) - 1);
    g_ssid_buffer[sizeof(g_ssid_buffer) - 1] = '\0';

    // Start connection attempt
    WiFi.begin(ssid, password);
    g_status = HAL_NETWORK_STATUS_CONNECTING;
    g_connect_start_ms = millis();

    return true;
}

// Retry tracking for auto-reconnect after ERROR
static uint8_t g_reconnect_attempts = 0;
static constexpr uint8_t MAX_RECONNECT_ATTEMPTS = 3;
static unsigned long g_error_time_ms = 0;
static constexpr unsigned long RECONNECT_BACKOFF_MS = 5000; // 5s between retries

hal_network_status_t hal_network_get_status(void) {
    // Update status based on WiFi state
    if (g_status == HAL_NETWORK_STATUS_CONNECTING) {
        wl_status_t wifi_status = WiFi.status();

        if (wifi_status == WL_CONNECTED) {
            g_status = HAL_NETWORK_STATUS_CONNECTED;
            g_reconnect_attempts = 0;  // Reset retry counter on success
            // Update SSID buffer from actual connection
            String ssid = WiFi.SSID();
            strncpy(g_ssid_buffer, ssid.c_str(), sizeof(g_ssid_buffer) - 1);
            g_ssid_buffer[sizeof(g_ssid_buffer) - 1] = '\0';
        } else if (wifi_status == WL_CONNECT_FAILED) {
            g_status = HAL_NETWORK_STATUS_ERROR;
            g_error_time_ms = millis();
        } else if (millis() - g_connect_start_ms > CONNECT_TIMEOUT_MS) {
            // Timeout — mark as error, disconnect without killing radio
            g_status = HAL_NETWORK_STATUS_ERROR;
            g_error_time_ms = millis();
            WiFi.disconnect(false);
        }
    } else if (g_status == HAL_NETWORK_STATUS_CONNECTED) {
        // Check if we've lost connection
        if (WiFi.status() != WL_CONNECTED) {
            g_status = HAL_NETWORK_STATUS_DISCONNECTED;
        }
    } else if (g_status == HAL_NETWORK_STATUS_ERROR && g_stored_ssid[0] != '\0') {
        // Auto-retry after error with backoff (handles post-suspend reconnect
        // failures where esp_wifi_stop tore down the driver and the first
        // WiFi.begin attempt timed out).
        if (g_reconnect_attempts < MAX_RECONNECT_ATTEMPTS &&
            millis() - g_error_time_ms > RECONNECT_BACKOFF_MS) {
            g_reconnect_attempts++;
            Serial.printf("[hal_network] Auto-retry %d/%d to %s\n",
                         g_reconnect_attempts, MAX_RECONNECT_ATTEMPTS, g_stored_ssid);
            ensureWiFiReady();
            WiFi.begin(g_stored_ssid, g_stored_password);
            g_status = HAL_NETWORK_STATUS_CONNECTING;
            g_connect_start_ms = millis();
        }
    }

    return g_status;
}

void hal_network_disconnect(void) {
    // Full shutdown: disconnect and stop the radio. Used before suspend/sleep
    // where we want the WiFi driver completely torn down.
    WiFi.disconnect(true);
    g_wifi_started = false;
    g_status = HAL_NETWORK_STATUS_DISCONNECTED;
    strncpy(g_ssid_buffer, "N/A", sizeof(g_ssid_buffer));
}

bool hal_network_reconnect(void) {
    if (g_stored_ssid[0] == '\0') {
        Serial.println("[hal_network_reconnect] No stored credentials");
        return false;
    }

    // Reset retry counter — this is a fresh reconnect (e.g., after resume)
    g_reconnect_attempts = 0;

    // After resume, WiFi was stopped by esp_wifi_stop() in hal_power_suspend().
    // ensureWiFiReady() handles the full restart since g_wifi_started was
    // cleared by hal_network_disconnect().
    ensureWiFiReady();
    WiFi.begin(g_stored_ssid, g_stored_password);
    g_status = HAL_NETWORK_STATUS_CONNECTING;
    g_connect_start_ms = millis();

    Serial.printf("[hal_network_reconnect] Reconnecting to %s...\n", g_stored_ssid);
    return true;
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

const char* hal_network_get_ssid(void) {
    if (WiFi.status() == WL_CONNECTED) {
        String ssid = WiFi.SSID();
        strncpy(g_ssid_buffer, ssid.c_str(), sizeof(g_ssid_buffer) - 1);
        g_ssid_buffer[sizeof(g_ssid_buffer) - 1] = '\0';
    } else {
        strncpy(g_ssid_buffer, "N/A", sizeof(g_ssid_buffer));
    }
    return g_ssid_buffer;
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
    Serial.printf("[hal_network_http_get] Buffer size: %zu bytes\n", buffer_size);

    // Begin HTTP connection
    Serial.println("[hal_network_http_get] Calling http.begin()...");
    bool begin_result = http.begin(url);
    if (!begin_result) {
        Serial.println("[hal_network_http_get] ERROR: http.begin() failed");
        return false;
    }
    Serial.println("[hal_network_http_get] http.begin() succeeded");

    http.setTimeout(10000);  // 10 second timeout

    // Explicitly request uncompressed content. Yahoo Finance (and many CDNs)
    // return gzip-compressed responses by default. The ESP32 HTTPClient does
    // NOT decompress gzip, so getString() returns raw compressed bytes —
    // appearing as garbled JSON with systematically dropped characters.
    http.addHeader("Accept-Encoding", "identity");

    Serial.println("[hal_network_http_get] Timeout set to 10000ms");

    Serial.println("[hal_network_http_get] Sending GET request...");
    int httpCode = http.GET();
    Serial.printf("[hal_network_http_get] GET returned code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
        Serial.println("[hal_network_http_get] HTTP_CODE_OK received");

        int contentLength = http.getSize();
        Serial.printf("[hal_network_http_get] Content-Length: %d bytes\n", contentLength);

        // Reject responses that exceed our buffer before reading
        if (contentLength > 0 && (size_t)contentLength >= buffer_size) {
            Serial.printf("[hal_network_http_get] ERROR: Response too large: %d bytes (buffer: %zu)\n",
                         contentLength, buffer_size);
            http.end();
            return false;
        }

        // Stream-based reading: read directly into our PSRAM buffer.
        // getString() builds an intermediate Arduino String char-by-char which
        // loses bytes under cross-core preemption. readBytes() copies the
        // decrypted TLS stream straight into the caller's buffer.
        WiFiClient* stream = http.getStreamPtr();
        if (!stream) {
            Serial.println("[hal_network_http_get] ERROR: No stream available");
            http.end();
            return false;
        }

        size_t totalRead = 0;
        size_t maxRead = (contentLength > 0) ? (size_t)contentLength : (buffer_size - 1);
        unsigned long lastDataTime = millis();

        while (totalRead < maxRead) {
            size_t avail = stream->available();
            if (avail > 0) {
                size_t toRead = min(avail, maxRead - totalRead);
                size_t bytesRead = stream->readBytes(response_buffer + totalRead, toRead);
                totalRead += bytesRead;
                lastDataTime = millis();
            } else if (!http.connected()) {
                break;  // Connection closed — we have all data
            } else if (millis() - lastDataTime > 10000) {
                Serial.println("[hal_network_http_get] Read timeout waiting for data");
                break;
            } else {
                delay(1);  // Yield while waiting for next chunk
            }
        }

        response_buffer[totalRead] = '\0';

        // Debug first 50 bytes
        Serial.print("[hal_network_http_get] First 50 bytes: ");
        size_t preview_len = min((size_t)50, totalRead);
        for (size_t i = 0; i < preview_len; i++) {
            Serial.printf("%02X ", (unsigned char)response_buffer[i]);
        }
        Serial.println();

        Serial.printf("[hal_network_http_get] SUCCESS: %zu bytes read via stream\n", totalRead);
        http.end();
        return (totalRead > 0);
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
