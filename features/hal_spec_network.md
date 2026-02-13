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
*   **Description:** Initializes the Wi-Fi hardware and starts an asynchronous connection attempt.
*   **Parameters:**
    *   `ssid`: The Wi-Fi network name.
    *   `password`: The Wi-Fi password.
*   **Returns:** `bool` - `true` if initialization was successful.

### `hal_network_get_status(void)`
*   **Description:** Returns the current connectivity status.
*   **Returns:** `hal_network_status_t`.

### `hal_network_ping(const char* host)`
*   **Description:** Performs a simple ICMP ping or HTTP HEAD request to verify internet connectivity.
*   **Returns:** `bool` - `true` if the host responded.
