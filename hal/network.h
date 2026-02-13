/**
 * @file network.h
 * @brief Hardware Abstraction Layer (HAL) - Network Specification
 *
 * This header defines the abstract interface for Wi-Fi and network connectivity.
 * See features/hal_spec_network.md for complete specification.
 */

#ifndef HAL_NETWORK_H
#define HAL_NETWORK_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network connection status
 */
typedef enum {
    HAL_NETWORK_STATUS_DISCONNECTED,  ///< Not connected to any network
    HAL_NETWORK_STATUS_CONNECTING,    ///< Connection attempt in progress
    HAL_NETWORK_STATUS_CONNECTED,     ///< Successfully connected to network
    HAL_NETWORK_STATUS_ERROR          ///< Connection error occurred
} hal_network_status_t;

/**
 * @brief Initializes Wi-Fi and starts connection attempt
 *
 * Initializes the Wi-Fi hardware and begins an asynchronous connection
 * to the specified network. Call hal_network_get_status() to check progress.
 *
 * @param ssid The Wi-Fi network name (SSID)
 * @param password The Wi-Fi password
 * @return true if initialization was successful, false otherwise
 */
bool hal_network_init(const char* ssid, const char* password);

/**
 * @brief Gets the current network connection status
 *
 * @return hal_network_status_t Current status
 */
hal_network_status_t hal_network_get_status(void);

/**
 * @brief Performs connectivity test to verify internet access
 *
 * Attempts to ping or perform an HTTP HEAD request to the specified host
 * to verify that internet connectivity is available.
 *
 * @param host Hostname or IP address to test (e.g., "8.8.8.8" or "google.com")
 * @return true if the host responded, false otherwise
 */
bool hal_network_ping(const char* host);

/**
 * @brief Performs an HTTP GET request and returns the response body
 *
 * Executes a blocking HTTP GET request to the specified URL and stores
 * the response body in the provided buffer.
 *
 * @param url The complete URL to fetch (e.g., "https://api.example.com/data")
 * @param response_buffer Buffer to store the response body (null-terminated string)
 * @param buffer_size Size of the response buffer in bytes
 * @return true if the request was successful (HTTP 200), false otherwise
 */
bool hal_network_http_get(const char* url, char* response_buffer, size_t buffer_size);

/**
 * @brief Explicitly disconnects from the current network
 */
void hal_network_disconnect(void);

/**
 * @brief Gets the SSID of the currently connected Wi-Fi network
 *
 * @return The SSID string, or "N/A" if not connected. Pointer is valid
 *         until the next call to this function.
 */
const char* hal_network_get_ssid(void);

#ifdef __cplusplus
}
#endif

#endif // HAL_NETWORK_H
