# Feature: HAL Network Specification

> Label: "HAL Network Specification"
> Category: "Hardware Layer"
> Prerequisite: features/hal_core_contract.md
> Prerequisite: features/arch_data_strategy.md

## Introduction

This document defines the abstract interface for Wi-Fi and network connectivity within the Hardware Abstraction Layer (HAL).

## Network Types

```c
typedef enum {
    HAL_NETWORK_STATUS_DISCONNECTED,
    HAL_NETWORK_STATUS_CONNECTING,
    HAL_NETWORK_STATUS_CONNECTED,
    HAL_NETWORK_STATUS_ERROR
} hal_network_status_t;
```

## Network HAL API

### `hal_network_init(const char* ssid, const char* password)`
*   **Description:** Starts an asynchronous connection attempt to a specific network. If already connected to another network, it should disconnect first.
*   **Parameters:**
    *   `ssid`: The Wi-Fi network name.
    *   `password`: The Wi-Fi password.
*   **Returns:** `bool` - `true` if initialization was successful.

## Connection Management Policy
1. **Iterative Boot:** The system must support an iterative connection strategy where it tries a list of credentials sequentially.
2. **Timeout:** Each attempt should have a non-blocking timeout mechanism to ensure the system doesn't hang on a single unavailable AP.
3. **Manual Override:** Any call to `hal_network_init` while a previous attempt is in progress should cancel the previous attempt and prioritize the new credentials.

### `hal_network_get_status(void)`
*   **Description:** Returns the current connectivity status.
*   **Returns:** `hal_network_status_t`.

### `hal_network_get_ssid(void)`
*   **Description:** Returns the SSID of the currently connected (or connecting) network.
*   **Returns:** `const char*` or `"NONE"`.

### `hal_network_disconnect(void)`
*   **Description:** Explicitly disconnects from the current network.

### `hal_network_ping(const char* host)`
*   **Description:** Performs a simple ICMP ping or HTTP HEAD request to verify internet connectivity.
*   **Returns:** `bool` - `true` if the host responded.
